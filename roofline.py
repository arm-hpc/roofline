#!/usr/bin/python3
# TODO: Force the usage of python3

"""
Roofline analysis tool. This tool implements the roofline methodology
as described in:

  "Cache-aware Roofline model: Upgrading the Loft",
  IEEE Computer Architecture Letters, January 2014

This implementation Copyright (C) ARM Ltd. 2019.  All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

Author: Andrea Brunato andrea.brunato@arm.com
"""

import argparse
import sys
import csv
import os
import math
import time
import statistics as st
import json
import subprocess as sp
from shutil import copyfile, move
import xml.etree.ElementTree as ET
from xml.etree.ElementTree import ElementTree
from time import gmtime, strftime
from decimal import Decimal

example_string = "roofline.py {record|report} <target_application>"
target_app_args = None
roofline_tool_dir = os.path.dirname(os.path.abspath(__file__)) + "/"
rooflines_db = roofline_tool_dir + "ert_rooflines/"
cwd = os.getcwd() + "/"

drrun = roofline_tool_dir + "/dynamorio/build/bin64/drrun "


class Point:
    def __init__(self, total_flops, color, app_name, total_time, total_bytes, read_bytes, write_bytes ,flops_per_byte, gflops_per_sec, label, start_line, end_line, start_src, end_src):
        self.total_flops = total_flops
        self.color = color
        self.app_name = app_name
        self.total_time = total_time
        self.total_bytes = total_bytes
        self.write_bytes = write_bytes
        self.read_bytes = read_bytes
        self.flops_per_byte = flops_per_byte
        self.gflops_per_sec = gflops_per_sec
        self.label = label
        self.start_line = start_line
        self.end_line = end_line
        self.start_src = start_src
        self.end_src = end_src

    def get_point_coordinates(self):
        return("  {} 	{}\n".format(self.flops_per_byte, self.gflops_per_sec))

    def add_point_label(self, out_dir):
        "Draw point on the plot"

        out_file = out_dir + "/roofline.gnu"
        # Add point label
        sed_add_label_info = "sed -i \"/output/a\set label \'{}\' at {},{} left textcolor rgb \'#000080\' offset 0.5,-0.5\" {}".format(
            self.label, self.flops_per_byte, self.gflops_per_sec, out_file)
        sp.call(sed_add_label_info, shell=True)

    def print_point(self):
        "Print point features to stdout"
        print("Point: Label: \'{}\' \n       {} FLOPs/Byte \n       {} Gflops/sec".format(
            self.label, self.flops_per_byte, self.gflops_per_sec))
        print("       App Name: " + format(self.app_name))
        print("       Total Time: {}".format(self.total_time))
        print("       Total Flops: " + format(self.total_flops, "e"))
        print("       Total Bytes: " + format(self.total_bytes, "e"))
        print("       Read Bytes: " + format(self.read_bytes, "e"))
        print("       Write Bytes: " + format(self.write_bytes, "e"))
        print("       Start line number: {}".format(self.start_line))
        print("       Start source file: {}".format(self.start_src))
        print("       End line number: {}".format(self.end_line))
        print("       End line number: {}\n".format(self.end_src))


def create_dat_file(out_dir, name, point_list):
    "Create a dat file which will be used by gnuplot to draw them"
    out_file = out_dir + "/" + name + ".dat"
    f = open(out_file, "w+")
    f.write("# X	Y\n")

    for p in point_list:
        f.write(p.get_point_coordinates())
    f.close()


def copy_dat_file(input_dir, name, output_dir):
    "Copy dat file into specified output directory"
    if not os.path.exists(output_dir + "/" + name + ".dat"):
        copyfile(input_dir + "/" + name + ".dat",
                 output_dir + "/" + name + ".dat")


def add_dat_file_reference(gnuplot_file, app_name, color):
    "Modifies gnuplot to point towards dat file for actually drawing the points in the chart"
    f = open(gnuplot_file, "rb+")
    f.seek(-1, os.SEEK_END)

    add_point_txt = ",\\\n\'{}\' title \'{}\' with points ls {} ".format(
        app_name + ".dat", app_name, color)
    f.write(add_point_txt.encode())
    f.close()


def get_points(in_dir, colour_n, name):
    "Get the point piece of information parsing the XML file"

    assert colour_n <= 5, "Please select less than 5 different files"

    root = ET.parse(in_dir + '/roofline.xml').getroot()
    root_time = ET.parse(in_dir + '/roofline_time.xml').getroot()
    point_list = []
    for p in root.findall('point'):
        label = p.attrib['label']
        app_flops = float(p.find('flops').text)
        app_bytes = float(p.find('bytes').text)
        read_bytes = float(p.find('read_bytes').text)
        write_bytes = float(p.find('write_bytes').text)
        src_file_start = p.find('src_file_start').text
        src_file_end = p.find('src_file_end').text
        line_start = int(p.find('line_n_start').text)
        line_end = int(p.find('line_n_end').text)
        # Get the correspondent element from the time xml file
        # Assert the label to be a unique ID, which must be present as well
        assert len(root_time.findall("point[@label='{}']".format(
            label))) == 1, "Label {} not found or present multiple times! Have you defined it correctly?".format(label)
        # Retrieve the timinig information corresponding to the same point from the timing file
        app_time = float(root_time.findall(
            "point[@label='{}']".format(label))[0].find('time').text)

        assert app_time != 0.0, "Your application runtime looks like to be zero"

        app_Gflops = app_flops / 1e9

        point_list.append(Point(
            total_flops=app_flops,
            app_name=name,
            color=colour_n,
            total_time=app_time,
            total_bytes=app_bytes,
            read_bytes=read_bytes,
            write_bytes=write_bytes,
            flops_per_byte=app_flops/app_bytes,
            gflops_per_sec=app_Gflops / app_time,
            label=label,
            start_line=line_start,
            end_line=line_end,
            start_src=src_file_start,
            end_src=src_file_end))

    return point_list


def ert_graph_is_available():
    "Checks out whether the Empirical Roofline Tool gnuplot is available"

    # TODO
    # This will check out whether the Empirical Tool has the roofline with the right precision
    if not os.path.exists(roofline_tool_dir + ".ert_results"):
        return False

    if not os.path.isfile(roofline_tool_dir + ".ert_results/roofline_ert.gnu"):
        return False

    return True


def run_client(app, options=[""]):
    "Run the roofline client on the target app with the given options"
    client_cmd = drrun + " -c {}/client/build/libroofline.so ".format(
        roofline_tool_dir) + " ".join(options) + " -- " + app
    print(client_cmd)
    sp.call(client_cmd, shell=True)


def run_roofline_client(args, app, out_dir=None):
    "Run the DynamoRIO client multiple times to gather all needed performance data"

    options = ["--output_folder " + out_dir if out_dir else ".",
               "--roi_start {}".format(args.roi_start) if args.roi_start else "",
               "--roi_end {}".format(args.roi_end) if args.roi_end else "",
               "--read_bytes_only" if args.read_bytes_only else "",
               "--write_bytes_only" if args.write_bytes_only else "",
               "--trace_f {}".format(args.trace_f) if args.trace_f else "",
               "--calls_as_separate_roi" if args.calls_as_separate_roi else ""]

    if args.flops_only:
        run_client(app, options=options)
        sys.exit()

    if args.time_only:
        options.append("--time_run")
        run_client(app, options=options)
        sys.exit()



    # Memory and FP Run
    run_client(app, options=options)

    # TODO: Wrap this in an appropriate function
    # Time Run: add the appropriate flag to communicate this to the DynamoRIO client
    options.append("--time_run")

    # Since time is the only metric which could be influenced by what is happening on the actual target machine,
    # we perform several statistical run to make sure the measurements are correct, if the user asks for it
    if args.run_time_analysis > 1:
        run_time_analysis(args, app, options, out_dir)
    else:
        run_client(app, options)




def run_time_analysis(args, app, options, out_dir):
    "Run multiple time the target application under different configuration for gathering a time analysis"

    ## Run the tool multiple times
    for i in range(0, args.run_time_analysis):
            run_client(app, options)
            move(out_dir + "roofline_time.xml", out_dir +
                    "roofline_time_{}.xml".format(i))


    ## Time measured by the tool
    time_dict = {}
    for i in range(0, args.run_time_analysis):
        root_time = ET.parse(
                out_dir + '/roofline_time_{}.xml'.format(i)).getroot()
        for p in root_time.findall('point'):
            label = p.attrib['label']
            time_info = float(p.find('time').text)
            if label in time_dict:
                time_dict[p.attrib['label']].append(time_info)
            else:
                time_dict[p.attrib['label']] = [float(time_info)]

    native_runtime = []
    for i in range(0, args.run_time_analysis):
        native_runtime.append(get_time_info(app, ""))

    dynamorio_runtime = []
    for i in range(0, args.run_time_analysis):
        dynamorio_runtime.append(get_time_info(app, drrun))

    dynamorio_and_client_runtime = []
    for i in range(0, args.run_time_analysis):
        start = time.time()
        run_client(app, options)
        end = time.time()
        dynamorio_and_client_runtime.append(end - start)

    #Single Points time
    save_time_point_statistics(time_dict, out_dir + "roi_runtime.csv")

    #"Native Total Time"
    save_time_statistics(native_runtime, out_dir + "native_runtime.csv")

    #"Roofline Detected timing: Total Time Under DynamoRIO"
    save_time_statistics(dynamorio_runtime, out_dir + "dynamorio_runtime.csv")

    #"Roofline Detected timing: Total Time Under DynamoRIO + roofline client"
    save_time_statistics(dynamorio_and_client_runtime, out_dir + "dynamorio_and_client_runtime.csv")




def save_time_point_statistics(point_info_dict, file_name):
    "Save timing info gathered for each point"
    with open(file_name, "w") as time_file:
        wr = csv.writer(time_file, quoting=csv.QUOTE_ALL)
        for l in point_info_dict:
            wr.writerow([l])
            for elem in point_info_dict[l]:
                wr.writerow([elem])


def save_time_statistics(time_info_list, file_name):
    "Save the timing information gathered across different runs in a csv file"
    with open(file_name, "w") as time_file:
        wr = csv.writer(time_file, quoting=csv.QUOTE_ALL)
        wr.writerow(["time"])
        for elem in time_info_list:
            wr.writerow([elem])


def get_time_info(app, settings):
    "Gather timing information for the given binary and environment settings."
    start = time.time()
    if settings:
        sp.call("./{}".format(app), shell=True)
    else:
        sp.call("{} ./{}".format(settings, app), shell=True)
    end = time.time()
    return end - start


def raw_statistic_analysis(lst):
    "Performs a really raw statistical analysis of the given list of numbers"
    print("Raw Data {}".format(lst))
    print("Mean {}".format(st.mean(lst)))
    print("Median {}".format(st.median(lst)))
    print("Variance {}".format(st.pvariance(lst)))
    print("Standar Deviation {}".format(st.stdev(lst)))



def show_roofline_ascii(out_dir, point_list):
    "If draw on shell is specified, use gnuplot to plot roofline on the terminal"
    ascii_file = out_dir + "/roofline_ascii.gnu"
    copyfile(out_dir + "/roofline.gnu", ascii_file)

    sed_replace_cmd = "sed -i \'/output/c\set terminal dumb\' {}".format(
        ascii_file)
    sp.call(sed_replace_cmd, shell=True)

    os.chdir(cwd + "/" + out_dir)
    sp.call("gnuplot roofline_ascii.gnu", shell=True)
    print("Regions of interests: \n")
    for p in point_list:
        p.print_point()

    # Display metainformation
    tree = ElementTree()
    root = tree.parse('roofline_meta.xml')
    for app in root.findall('application'):
        print("Command: \'{}\' executed at {}".format(
            app.find('name').text, app.find('time').text))


def get_and_save_metainfo(input_dir_lst, out_dir):
    "Displays metainformation about the specified commands"
    tree = ElementTree()
    root = tree.parse(input_dir_lst[0] + '/roofline_meta.xml')
    for in_dir in input_dir_lst[1:]:
        root_it = ET.parse(in_dir + '/roofline_meta.xml').getroot()
        root.append(root_it.find('application'))
    tree.write(out_dir + '/roofline_meta.xml')


def get_app_title(in_dir):
    root = ET.parse(in_dir + '/roofline_meta.xml')
    return root.find('application').find('name').text.replace(" ", "_")


def checkout_input_args(args):
    "Checkout that the input parameters actually make sense."

    assert args.input_dir is not None, "ERROR: Please specify the input directory using the -i flag"
    assert args.line is not None, "ERROR: Please specify the roofline-like line (or lines) you want to use."

    for in_dir in args.input_dir:
        assert os.path.isfile(
            in_dir + "/roofline.xml"), "File roofline.xml in {} not found. Have you previously run roof record? ".format(in_dir)
        assert os.path.isfile(
            in_dir + "/roofline_time.xml"), "File roofline_time.xml in {} directory not found. Have you previously run roof record? ".format(in_dir)

    # TODO: Add some checking to actually see whether gnuplot is available, otherwise throw a meaningful error

    if len(args.input_dir) > 1:
        assert args.output_dir is not None, "Please specify an output directory"

    # If the output direcroty is not specified, store the output of roof report in the input one.
    if args.output_dir is None and len(args.input_dir) == 1:
        args.output_dir = args.input_dir[0]

def add_additional_lines(gnuplot_file, line_lst):
    "Add additional roofline-like lines to the chart"   
    #Adding ,\ at the end of file.
    f = open(gnuplot_file, "rb+")
    f.seek(-1, os.SEEK_END)
    add_point_txt = ",\\\n"
    f.write(add_point_txt.encode())
    f.close()

    ##Open the same file again in append mode, adding the lines we want to plot
    f = open(gnuplot_file, "a")

    # Copy roofline lines from the specified input files
    for lines in line_lst:
        print("Adding additional lines ({}) in the plot".format(lines))
        #Open the corresponding file
        source_file = rooflines_db + lines + "/roofline.gnu" 
        for i in range(3,0,-1):
            proc = sp.Popen("tail -n{} {} | head -n1".format(i, source_file), shell=True, stdout= sp.PIPE)
            output = proc.communicate()[0]
            ##Add the line to the output gnuplot
            f.write(output.decode("utf-8"))
        f.close()
        ## Pick up the corresponding labels
        sed_get_labels = "sed -n \'/^set label/p\' {}".format(source_file)
        proc = sp.Popen(sed_get_labels, shell=True, stdout=sp.PIPE)
        output = proc.communicate()[0]
        labels = output.decode("utf-8").split('\n')
        print("Labels are: {}".format(labels))
        ## Add labels to destination file
        for l in labels:
            if l:
                sed_add_labels = "sed -i \"/plot/i \{}\" {} ".format(l, gnuplot_file)
                print(sed_add_labels)
                sp.call(sed_add_labels, shell=True)




def report(args):
    "Report the gathered performance by drawing the plot"
    # If the output directory doesn't exists, create it

    checkout_input_args(args)

    if not os.path.exists(args.output_dir):
        os.makedirs(args.output_dir)

    for l in args.line:
        if not os.path.exists(rooflines_db + l):
            print("Roofline for {} does not exists. Have you run ./roofline.py record_ert --precision <Precision>?".format(l))

    if len(args.line) == 1:
        print("Roofline: building report merging a roofline for {} precision. Please make sure you target application has used the same precision".format(args.line[0]))

    copy_empty_roofline(rooflines_db + args.line[0],  args.output_dir, args.title)

    if len(args.line) > 1:
        add_additional_lines(args.output_dir + "/roofline.gnu", args.line[1:])

    # Parse the XML file and compute all the info about the points that have to be plotted.
    point_list = []
    for colour_n, in_dir in enumerate(args.input_dir):
        # Get points from the given input directory
        current_points = get_points(in_dir, colour_n+1, get_app_title(in_dir))
        # Create its associated dat file in the given input directory.
        create_dat_file(in_dir, get_app_title(in_dir), current_points)
        # Copy the dat file onto the output directory
        copy_dat_file(in_dir, get_app_title(in_dir), args.output_dir)
        # Overwrite roofline.gnu to actually draw the points from the dat file
        add_dat_file_reference(
            args.output_dir + "/roofline.gnu", get_app_title(in_dir), colour_n+1)
        # Update all point list
        point_list = point_list + current_points

    get_and_save_metainfo(args.input_dir, args.output_dir)

    for p in point_list:
        p.add_point_label(args.output_dir)

    if not args.no_shell_plot:
        show_roofline_ascii(args.output_dir, point_list)

    # Run gnuplot to draw the actual plot as well.
    print(cwd + "/" + args.output_dir)
    os.chdir(cwd + "/" + args.output_dir)
    sp.call("gnuplot {}".format("./roofline.gnu"), shell=True)


def get_roofline_hostname(roofline_folder):
    "Retrieves the hostname from the roofline plot has been gathered."
    metainfo = open(roofline_folder + "/roofline.json")
    roofline_data = json.load(metainfo)
    return str(roofline_data['empirical']['metadata']['HOSTNAME'][0])


def get_roofline_precision(roofline_folder):
    "Retrives the floating point precision from the given roofline plot"
    metainfo = open(roofline_folder + "/roofline.json")
    roofline_data = json.load(metainfo)
    return str(roofline_data['empirical']['metadata']['CONFIG']['ERT_PRECISION'][0])


def show_ert(args):
    "Displays the available rooflines"

    dir_list = os.listdir(rooflines_db)
    for roofs in dir_list:
        hostname, precision = roofs.split("_")
        print("Hostname {} - Precision {}".format(hostname, precision))



def record_ert(args):
    "Run the Empirical Roofline Tools for getting more information about the current machine"

    print("Running ERT for gathering a roofline for precision {}".format(args.precision))
    os.chdir(roofline_tool_dir + "/ert/Empirical_Roofline_Tool-1.1.0/")

    # Copy the ert template configuration file with the the user specified settings.
    copyfile("roofline_config_template", "roofline_config")
    # Set precision
    sed_precision_cmd = "sed -i '/ERT_PRECISION/c\ERT_PRECISION {}' {}".format(
        args.precision, "roofline_config")
    sp.call(sed_precision_cmd, shell=True)

    # Run the Ert tool.
    sp.call("./ert roofline_config", shell=True)

    # Copy ert plot into roofline own version
    if not os.path.exists(roofline_tool_dir + "ert_rooflines"):
        os.makedirs(roofline_tool_dir + "ert_rooflines")
    ert_output_folder = "./Roofline/Run.001/"
    # Move the whole roofline directory into the internal Roofline Tool Database folder, renaming it as hostname+precision
    move(ert_output_folder,
         roofline_tool_dir + "/ert_rooflines/" + get_roofline_hostname(ert_output_folder) + "_" + get_roofline_precision(ert_output_folder))
    os.chdir(cwd)


def save_meta_information(args, app, out_dir):
    # Log in a file metainformation about the command being run.
    f = open(out_dir + "roofline_meta.xml", "w+")
    f.write("<?xml version=\"1.0\"?>\n")
    f.write("<metainfo>\n")
    f.write("<application>\n")
    f.write("<name>{}</name>\n".format(app))
    f.write("<time>{}</time>\n".format(strftime("%Y-%m-%d %H:%M:%S", gmtime())))
    f.write("</application>\n")
    f.write("</metainfo>\n")
    f.close()


def copy_empty_roofline(roofline_folder_plot, out_dir, title):
    # Here ,theoretically speaking, you could copy whatever you want.
    # Dat files as well?

    out_file = out_dir + "/roofline.gnu"
    copyfile(roofline_folder_plot + "/roofline.gnu", out_file)

    # TODO: Why not, copy over the whole directory as well.

    # Enlarge Y axis
    sed_adjust_y_scaling_cmd = r"sed -i '/yrange/c\set yrange [1.000000e-02 : 1.000000e+02] noreverse nowriteback' {}".format(
        out_file)
    sp.call(sed_adjust_y_scaling_cmd, shell=True)

    # Output for ps.
    sed_replace_cmd = "sed -i \'/output/c\set output \"roofline.ps\"\' {}".format(
        out_file)
    sp.call(sed_replace_cmd, shell=True)

    # Add some styling for the points which will be drawn with the report command
    sed_add_line_style_cmd = "sed -i \"/output/a \set style line 1 lc rgb 'blue' pt 7\" {} ".format(
        out_file)
    sp.call(sed_add_line_style_cmd, shell=True)
    sed_add_line_style_cmd = "sed -i \"/output/a \set style line 2 lc rgb 'green' pt 7\" {} ".format(
        out_file)
    sp.call(sed_add_line_style_cmd, shell=True)
    sed_add_line_style_cmd = "sed -i \"/output/a \set style line 3 lc rgb 'red' pt 7\" {} ".format(
        out_file)
    sp.call(sed_add_line_style_cmd, shell=True)
    sed_add_line_style_cmd = "sed -i \"/output/a \set style line 4 lc rgb 'orange' pt 7\" {} ".format(
        out_file)
    sp.call(sed_add_line_style_cmd, shell=True)
    sed_add_line_style_cmd = "sed -i \"/output/a \set style line 5 lc rgb 'black' pt 7\" {} ".format(
        out_file)
    sp.call(sed_add_line_style_cmd, shell=True)

    # Add title if specified
    if title is not None:
        sed_change_title = "sed -i \'/set title/c\set title \"{}\" \' {}".format(
            title, out_file)
        sp.call(sed_change_title, shell=True)

    # Add command to add legend
    sed_add_legend = "sed -i \"/output/a \set key top right\" {} ".format(
        out_file)
    sp.call(sed_add_legend, shell=True)

    # Remove grid
    sed_remove_grid = "sed -i \'/set grid xtics/,+1 d\' {}".format(out_file)
    sp.call(sed_remove_grid, shell=True)


def record(args):
    "Use the Roofline client to record the target app performance"
    # Build up the command for running the target application with its arguments

    global target_app_args
    if target_app_args:
        target_app_args = " ".join(target_app_args)
        app_with_flags = args.target_app + " " + target_app_args
    else:
        app_with_flags = args.target_app

    # If the app is run as "./<app>", remove the initial ./
    if app_with_flags[0:2] == "./":
        app_with_flags = app_with_flags[2:]

    # Check out the output directory does not already exists
    out_dir = args.output if args.output else app_with_flags.replace(
        " ", "_") + strftime("%Y-%m-%d_%H:%M:%S", gmtime())
    out_dir = cwd + out_dir + "/"

    if not os.path.exists(out_dir):
        os.makedirs(out_dir)
    else:
        print("You've specified an already existing output directory. Please specify a different destination\n")
        sys.exit()

    save_meta_information(args, app=app_with_flags, out_dir=out_dir)

    # Run the Roofline client on the target application
    run_roofline_client(args, app=app_with_flags, out_dir=out_dir)


def main():
    parser = argparse.ArgumentParser(
        prog='roofline', description="Roofline Tool. Example usage: \n" + example_string)
    subparsers = parser.add_subparsers(
        help='Choose either record or report for recording your application and reporting it in a roofline chart', dest='subparser_name')
    # Record
    record_parser = subparsers.add_parser('record', help='Record the specified application in its execution. Please make sure to define the Region of Interest for your program.'
                                          'You can specify a region of interest in 3 different ways:\n\n -> Recompiling the target application using the provided include file\n'
                                          '-> Designating functions already present in your app as delimiters by specifying --roi_start <Function Name> --roi_end <Function Name>\n'
                                          '-> Designating a whole function body as a roi, by specifying --trace_f <Function Name>\n')
    record_parser.add_argument('target_app')
    record_parser.add_argument(
        '--output', '-o', help='Output file name for saving the gathered performance information')
    record_parser.add_argument(
        '--roi_start', help='Specify the function name inside the binary which delimits the beginning of the Region of Interest (ROI)')
    record_parser.add_argument(
        '--roi_end', help='Specify the function name inside the binary which delimits the end of the Region of Interest (ROI)')
    record_parser.add_argument(
        '--trace_f', help='Specify the function name whose whole execution will be taken into account as a Region of Interest')
    record_parser.add_argument(
        '--calls_as_separate_roi', help='To be used only after specifying --trace_f, takes into account each function execution as a different ROI', action='store_true')
    record_parser.add_argument('--run_time_analysis', type=int, default=1,
                               help='Run a statistic analysis on the timing information gathered by the client.')
    record_parser.add_argument(
        '--time_only', help='Run the roofline client to get timing information only', action='store_true')
    record_parser.add_argument(
        '--read_bytes_only', help='Take into account only bytes which are being read', action='store_true')
    record_parser.add_argument(
        '--write_bytes_only', help='Take into account only bytes which are being written', action='store_true')
    record_parser.add_argument(
        '--flops_only', help='Run the roofline client to get flops and bytes information only', action='store_true')
    record_parser.set_defaults(func=record)

    # Report
    report_parser = subparsers.add_parser(
        'report', help='Report the gathered information by presenting the Roofline Chart')
    report_parser.add_argument(
		    '--line', nargs='+', help='Specify the Roofline-shaped line(s) you want to use for plotting the results. Format to be used is: <HOSTNAME>_<PRECISION>. In order to see the already gathered lines, run >roofline show ert')
    report_parser.add_argument('--input_dir', '-i', nargs='+',
                               help='Input folder (previously generate via roof record)')
    report_parser.add_argument(
        '--output_dir', '-o', help='Output folder, where to store the report')
    report_parser.add_argument(
        '--no_shell_plot', help='Do not plot Roofline on the shell', action='store_true')
    report_parser.add_argument(
        '--title', help='Define a title for the roofline chart')
    report_parser.set_defaults(func=report)

    # Record ERT
    record_ert_parser = subparsers.add_parser(
        'record_ert', help='Run the empirical roofline tool for recording roofline')
    record_ert_parser.add_argument('--precision', help='Specify the roofline precision values. Options: FP64, FP32 ...',
                                   default='FP64', const='FP64', nargs='?', choices=['FP64', 'FP32'])
    record_ert_parser.set_defaults(func=record_ert)

    # Show ERT Rooflines
    show_ert_parser = subparsers.add_parser(
        'show_ert', help='Shows available roolines, displaying information about the host where they have been created and their precision')
    show_ert_parser.set_defaults(func=show_ert)


    # Get option flags for the target application
    global target_app_args
    args, target_app_args = parser.parse_known_args()

    # Call the function corresponding to the given action {record, report}
    args.func(args)


if __name__ == '__main__':
    sys.exit(main())
