#!/usr/bin/python3
import getopt
import trace
import traceback
import sys
import os
import shutil

from string import Template
from datetime import datetime


project_name = ""
proto_file = ""
out_project_path = "./"
bin_path = ""
conf_path = ""
test_client_path = ""
test_client_tool_path = ""
src_path = ""
build_type =""

generator_path = sys.path[0]


def to_camel(input_s):
    if input_s.find('_') == -1:
        return input_s
    re = ''
    for s in input_s.split('_'):
        re += s.capitalize()
    return re

def to_underline(input_s):
    tmp = to_camel(input_s)
    re = ''
    for s in tmp:
        re += s if s.islower() else '_' + s.lower()
    re = re[1:]
    return re



def parseInput():
    global build_type
    build_type = "makefile"

    opts,args = getopt.getopt(sys.argv[1:], "hi:o:b:", ["help", "input=", "output= ","build_type="] )

    if len(sys.argv) < 2:
        print("Usage: python rocket_generator.py -i <input> -o <output>")
        print("Example: python rocket_generator.py -i ./rocket.proto -o ./rocket")
        sys.exit(1)
    for opt, arg in opts:

        if opt in ("-h", "--help"):
            printHelp()
            sys.exit(0)
        if opt in ("-i", "--input"):
            global proto_file
            proto_file = arg
        elif opt in ("-o", "--output"):
            global out_project_path
            out_project_path = arg
            if out_project_path[-1] != '/':
                out_project_path += '/'
        elif opt in ("-b", "--build_type"):
            build_type = arg.lower()
            if build_type not in ["cmake", "makefile"]:
                raise Exception("Invalid build type: {build_type} upported types: makefile, cmake")
        else:
            raise Exception("Invalid argument: [" + opts + ": " + arg + "]" ) 
    if not os.path.exists(proto_file):
        raise Exception("Generate error, not exist protobuf file: " + proto_file)
    
    if ".proto" not in proto_file:
        raise Exception("Generate error, input file is't standard protobuf file:[ " + proto_file + "]")

    global project_name
    project_name = proto_file[0: -6]
    print(f"project_name:" + project_name)
    print(f"Build type selected: {build_type}")  # 输出构建类型信息

def printHelp():

    print('=' * 100)
    print('Welcome to use Rocket Generator, this is help document:\n')
    print('Run Environment: Python(version 3.6 or high version is better).')
    print('Run Platform: Linux Only(kernel version >= 3.9 is better).')
    print('Others: Only protobuf3 support, not support protobuf2.\n')
    print('Usage:')
    print('rocket_generator.py -i <input> -o <output> [-b <build_type>]\n')
    print('Options:')
    print('-h, --help')
    print(('    ') + 'Print help document.\n')

    print('-i xxx.proto, --input xxx.proto')
    print(('    ') + 'Input the target proto file, must standard protobuf3 file, not support protobuf2.\n')

    print('-o dir, --output dir')
    print(('    ') + 'Set the path that your want to generate project, please give a dir param.\n')

    print('-b type, --build_type type')
    print(('    ') + 'Set the build type, support makefile and cmake, default is makefile.\n')
    print(('    ') + 'If you want to use cmake, please make sure your cmake version >= 3.0.\n')

    print('')
    print('For example:')
    print('rocket_generator.py -i order_server.proto -o ./ -b cmake')

    print('')
    print('=' * 100)  

def generate_dir():
    print('='* 100)
    print("Begin to generate project dir")
    if out_project_path == "":
        proj_path = './' + project_name
    if out_project_path[-1] == '/':
        proj_path = out_project_path + project_name.strip()
    else:
        proj_path = out_project_path + './' + project_name.strip()

    global bin_path
    bin_path = proj_path + '/bin'

    global conf_path
    conf_path = proj_path + '/conf'

    global test_client_path
    test_client_path = proj_path + '/test_client'

    global test_client_tool_path
    test_client_tool_path = test_client_path + '/test_tool'


    log_path = proj_path + '/log'
    lib_path = proj_path + '/lib'
    obj_path = proj_path + '/obj'

    global src_path
    src_path = proj_path + '/' + project_name
    src_interface_path = src_path + '/interface'
    src_service_path = src_path + '/service'
    src_pb_path = src_path + '/pb'
    src_comm_path = src_path + '/comm'

    dir_list = [proj_path, bin_path, conf_path, log_path, lib_path, obj_path,test_client_path, test_client_tool_path, src_path, src_interface_path, src_service_path, src_pb_path, src_comm_path]

    for dir in dir_list:
        if not os.path.exists(dir):
            os.makedirs(dir)
            print("success make dir in " + dir)
    
    print("end gernerate project dir")
    print('=' * 100)




def generate_pb():
    print('='* 100)
    print("Begin to generate protobuf file")
    pb_path = src_path + '/pb/'
    cmd = 'cp -r ' + proto_file + ' ' + pb_path
    cmd += ' && cd ' + pb_path + ' && protoc --cpp_out=. ' + proto_file
    print("execute cmd: " + cmd)

    if os.system(cmd) != 0:
        raise Exception("generate protobuf file failed, cmd:[" + cmd + "]")
    
    print("end generate protobuf file")
    print('='* 100)


def generate_makefile():
    print('='* 100)
    print("Begin to generate makefile")
    out_file = src_path + '/makefile'
    if os.path.exists(out_file):
        print("makefile already exist, skip generate")
        print("End generate makefile")
        print('='* 100)
        return
    
    template_file = open(generator_path + '/template/makefile.template', 'r')
    tmpl = Template(template_file.read())


    content = tmpl.safe_substitute(
        PROJECT_NAME = project_name,
        CREATE_TIME = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    )

    file = open(out_file, 'w')
    file.write(content)
    file.close()
    print("success generate makefile in " + out_file)
    print("end generate makefile")
    print('='* 100)

def generate_cmake_file():
    print('='* 100)
    print("Begin to generate CMakeLists.txt")
    
    out_file = os.path.join(project_name, 'CMakeLists.txt')
    
    if os.path.exists(out_file):
        print("CMakeLists.txt already exists, skip generate")
        print("End generate CMakeLists.txt")
        print('='* 100)
        return
    
    
    template_path = os.path.join(generator_path, 'template/CMakeLists.txt.template')
    try:
        with open(template_path, 'r') as template_file:
            tmpl = Template(template_file.read())
    except FileNotFoundError:
        print(f"Error: CMake template not found at {template_path}")
        return

    
    content = tmpl.safe_substitute(
        PROJECT_NAME=project_name,
        CREATE_TIME=datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
        ROCKET_LIB="/usr/lib/librocket.a",  
        PROTOBUF_LIB="/usr/lib/libprotobuf.a",
        TINYXML_LIB="/usr/lib/libtinyxml.a"
    )

    
    try:
        with open(out_file, 'w') as f:
            f.write(content)
        print(f"Successfully generated CMakeLists.txt at {out_file}")
    except IOError as e:
        print(f"Error writing CMakeLists.txt: {str(e)}")
    
    print("End generate CMakeLists.txt")
    print('='* 100)

def generate_run_script():
    print('='* 100)
    print("Begin to generate run script")
    dir_src = generator_path + '/template/'
    cmd = 'cp -r ' + dir_src + '*.sh ' + bin_path + "/"
    print("execute cmd: " + cmd)
    os.system(cmd)

    print("End generate run script")
    print('='* 100)
 
def generate_conf_file():
    print('='* 100)
    file_name = "rocket.xml"
    print("Begin to generate conf file")
    out_file = conf_path + '/' + file_name
    if os.path.exists(out_file):
        print("conf file already exist, skip generate")
        print("End generate conf file")
        print('='* 100)
        return
    template_file = open(generator_path + '/template/conf.xml.template', 'r')

    tmpl = Template(template_file.read())
    content = tmpl.safe_substitute(
        PROJECT_NAME = project_name,
        FILE_NAME = file_name,
        CREATE_TIME = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    )

    file = open(out_file, 'w')
    file.write(content)
    file.close()
    print("success write to " + out_file)
    print("End generate rocket conf file")
    print('='* 100)


def generate_framework_code():
    print('='* 100)
    print("Begin to generate framework code")

    pb_head_file = src_path + '/pb/' + project_name + '.pb.h'
    file = open(pb_head_file, 'r')
    origin_text = file.read()

    begin = origin_text.find('virtual ~')
    i1 = origin_text[begin:].find('~')
    i2 = origin_text[begin:].find('(')
    service_name = origin_text[begin + i1 + 1 : begin + i2]
    print("service_name: " + service_name)

    origin_text = origin_text[begin + i2: ]
    method_list = []

    i1 = 0

    while True:
        i1 = origin_text.find('virtual void')
        if (i1 == -1):
            break
        i2 = origin_text[i1:].find(');')
        method_list.append(origin_text[i1: i1 + i2 + 2])
        origin_text = origin_text[i1 + i2 + 3: ]

    print('=' * 100)
    print("Begin to generate business_exception.h")
    exception_file = src_path + '/comm/business_exception.h'
    if not os.path.exists(exception_file):
        #generate business_exception.h
        exception_template_file = Template(open(generator_path + '/template/business_exception.h.template', 'r').read())
        exception_content = exception_template_file.safe_substitute(
            PROJECT_NAME = project_name,
            FILE_NAME = 'business_exception.h',
            HEADER_DEFINE = project_name.upper() + '_COMM_BUSINESSEXCEPTION_H',
            CREATE_TIME = datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
            INCLUDE_INTERFACEBASE_HEADER_FILE = '#include "' + project_name + '/interface/interface.h"',
        )
        out_exception_file = open(exception_file, 'w')
        out_exception_file.write(exception_content)
        out_exception_file.close()
        print("success copy business_exception.h to " + exception_file)
    else:
        print("file:[" + exception_file + "] already exist, skip copy")

    print("End generate business_exception.h")
    print('=' * 100)

    print('=' * 100)
    print("Begin to generate server.h")
    class_name = to_camel(service_name) + 'Impl'
    head_file_template = Template(open(generator_path + '/template/server.h.template', 'r').read())

    head_file_content = head_file_template.safe_substitute(
        HEADER_DEFINE = project_name.upper() + '_SERVICE_' + project_name.upper() + '_H',
        FILE_NAME = project_name + '.h',
        PROJECT_NAME = project_name,
        CLASS_NAME = class_name,
        SERVICE_NAME = service_name,
        PB_HEAD_FILE = project_name + '/pb/' + project_name + '.pb.h',
        CREATE_TIME = datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
        INCLUDE_PB_HEADER = '#include "' + project_name + '/pb/' + project_name + '.pb.h"',
    )
    i1 = head_file_content.find('${METHOD}')
    pre_content = head_file_content[0: i1]
    next_content = head_file_content[i1 + 9: ]
    for each in method_list:
        each = each.replace('PROTOBUF_NAMESPACE_ID', 'google::protobuf')
        pre_content += '// override from ' + service_name + '\n  '
        pre_content += each
        pre_content += '\n\n  '
        
    content = pre_content + next_content
    out_head_file = open(src_path + '/service/' + project_name + '.h', 'w')
    out_head_file.write(content)
    out_head_file.close()

    print('End generate server.h')
    print('=' * 100)

    print('=' * 100)
    print("Begin to generate server.cpp")
    cc_file_template = Template(open(generator_path + '/template/server.cpp.template', 'r').read())
    cc_file_content = cc_file_template.safe_substitute(
        FILE_NAME = project_name + '.cpp',
        PROJECT_NAME = project_name,
        INCLUDE_PB_HEADER = '#include "' + project_name + '/pb/' + project_name + '.pb.h"', 
        INCLUDE_BUSINESS_EXCEPTION_HEADER = '#include "' + project_name + '/comm/business_exception.h"',
        INCLUDE_SERVER_HEADER = '#include "' + project_name + '/service/' + project_name + '.h"', 
        CREATE_TIME = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    )
    
    method_i = cc_file_content.find('${METHOD}')
    pre_content = cc_file_content[0: method_i]
    next_content = cc_file_content[method_i + 9: ]
    interface_list = []

    for each in method_list:
        tmp = each.replace('PROTOBUF_NAMESPACE_ID', 'google::protobuf')
        i1 = tmp.find('void')
        tmp = tmp[i1:]

        i2 = tmp.find( '(' )
        method_name = tmp[5:i2]
        interface_class_name = to_camel(method_name) + 'Interface'
        interface_file_name = to_underline(method_name)
        l = tmp.split(',')
        y = l[1].find('request')
        request_type = l[1][0: y - 1].replace('*', '').replace('const ', '').replace('\n', '').replace(' ', '')
        print(request_type)

        y = l[2].find('response')
        response_type = l[2][0: y - 1].replace('*', '').replace('\n', '').replace(' ', '')
        print(response_type)

        interface_list.append({
            'interface_name': interface_file_name,
            'method_name': method_name,
            'interface_class_name': interface_class_name,
            'request_type': request_type,
            'response_type': response_type
        })
        print(interface_list)

        tmp = tmp[0: 5] + class_name + '::' + tmp[5:]
        tmp = tmp[0: -1] + '{\n\n  ' + 'CALL_RPC_INTERFACE(' + interface_class_name + ');\n}'
        print('tmp: ' + tmp)
        pre_content += tmp
        pre_content += '\n\n'

    include_str = ''
    for each in interface_list:
        include_str += '#include "' + project_name + '/interface/' + each['interface_name'] + '.h"\n'
    pre_content = pre_content.replace('${INCLUDE_SERVICE}', include_str)

    out_cc_file = open(src_path + '/service/' + project_name + '.cpp', 'w')
    out_cc_file.write(pre_content + next_content)
    out_cc_file.close()
    print('End generate server.cpp')
    print('=' * 100)

    print('=' * 100)
    print("Begin to generate main.cpp")

    # genneator main.cpp file
    main_file = src_path + '/main.cpp'
    if not os.path.exists(main_file):
        main_file_temlate = Template(open(generator_path + '/template/main.cpp.template','r').read())
        main_file_content = main_file_temlate.safe_substitute(
            FILE_NAME = project_name + '.h',
            PROJECT_NAME = project_name,
            CLASS_NAME = project_name + "::" + class_name,
            INCLUDE_SERVER_HEADER = '#include "service/' + project_name + '.h"', 
            CREATE_TIME = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        )
        main_file_handler = open(main_file, 'w')
        main_file_handler.write(main_file_content)
        main_file_handler.close()
    else:
        print("file: [" + main_file + "] exist, skip")

    print('End generate main.cpp')
    print('=' * 100)

    print('=' * 100)
    # genneator interface.h file
    interfase_base_h_file = src_path + '/interface/interface.h'
    if not os.path.exists(interfase_base_h_file):
        interface_base_h_file_temlate = Template(open(generator_path + '/template/interface_base.h.template','r').read())
        interfase_base_h_file_content = interface_base_h_file_temlate.safe_substitute(
            FILE_NAME = 'interface.h',
            PROJECT_NAME = project_name,
            HEADER_DEFINE = project_name.upper() + '_INTERFACE_H',
            CREATE_TIME = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        )
        interface_base_h_handler = open(interfase_base_h_file, 'w')
        interface_base_h_handler.write(interfase_base_h_file_content)
        interface_base_h_handler.close()
    else:
        print("file: [" + interfase_base_h_file + "] exist, skip")

    print('End generate interface.h')
    print('=' * 100)

    # genneator interface.cpp file
    interfase_base_cc_file = src_path + '/interface/interface.cpp'
    if not os.path.exists(interfase_base_cc_file):
        interface_base_cc_file_temlate = Template(open(generator_path + '/template/interface_base.cpp.template','r').read())
        interfase_base_cc_file_content = interface_base_cc_file_temlate.safe_substitute(
            FILE_NAME = 'interface.cpp',
            PROJECT_NAME = project_name,
            CREATE_TIME = datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
            INCLUDE_INTERFACEBASE_HEADER_FILE = '#include "' + project_name + '/interface/interface.h"',
        )
        interface_base_cc_handler = open(interfase_base_cc_file, 'w')
        interface_base_cc_handler.write(interfase_base_cc_file_content)
        interface_base_cc_handler.close()
    else:
        print("file: [" + interfase_base_cc_file + "] exist, skip")

    print('End generate interface.cpp')
    print('=' * 100)

    print('=' * 100)
    print('Begin generate each rpc method interface.cpp & interface.h')
    # genneator each interface.cpp and .h file
    interface_head_file_temlate = Template(open(generator_path + '/template/interface.h.template','r').read())
    interface_cc_file_temlate = Template(open(generator_path + '/template/interface.cpp.template','r').read())
    interface_test_client_file_template = Template(open(generator_path + '/template/test_rocket_client.cpp.template','r').read())

    stub_name = service_name + "_Stub"
    for each in interface_list:

        # interface.h 
        file = src_path + '/interface/' + each['interface_name'] + '.h'
        if not os.path.exists(file):
            header_content = interface_head_file_temlate.safe_substitute(
                PROJECT_NAME = project_name,
                INCLUDE_PB_HEADER = '#include "' + project_name + '/pb/' + project_name + '.pb.h"', 
                HEADER_DEFINE = project_name.upper() + '_INTERFACE_' + each['interface_name'].upper() + '_H',
                CREATE_TIME = datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
                CLASS_NAME = each['interface_class_name'],
                REQUEST_TYPE = each['request_type'],
                RESPONSE_TYPE = each['response_type'], 
                INCLUDE_INTERFACEBASE_HEADER_FILE = '#include "' + project_name + '/interface/interface.h"',
                FILE_NAME = each['interface_name'] + '.h'
            )
            out_interface_header_file = open(file, 'w')
            out_interface_header_file.write(header_content)
            out_interface_header_file.close()
        else:
            print("file: [" + file + "] exist, skip")

        # interface.cpp 
        file = src_path + '/interface/' + each['interface_name'] + '.cpp'
        if not os.path.exists(file):
            cc_file_content = interface_cc_file_temlate.safe_substitute(
                PROJECT_NAME = project_name,
                INCLUDE_PB_HEADER = '#include "' + project_name + '/pb/' + project_name + '.pb.h"', 
                INCLUDE_INTERFACE_HEADER_FILE = '#include "' + project_name + '/interface/' + each['interface_name'] + '.h"',
                INCLUDE_INTERFACEBASE_HEADER_FILE = '#include "' + project_name + '/interface/interface.h"',
                CREATE_TIME = datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
                CLASS_NAME = each['interface_class_name'],
                REQUEST_TYPE = each['request_type'],
                RESPONSE_TYPE = each['response_type'],
                FILE_NAME = each['interface_name'] + '.cpp'
            )
            out_interface_cc_file = open(file, 'w')
            out_interface_cc_file.write(cc_file_content)
            out_interface_cc_file.close()
        else:
            print("file: [" + file + "] exist, skip")
        
        # test_interface_client.cpp
        file = test_client_path + '/test_' + each['interface_name'] + '_client.cpp'
        if not os.path.exists(file):
            cc_file_content = interface_test_client_file_template.safe_substitute(
                INCLUDE_PB_HEADER = '#include "' + project_name + '/pb/' + project_name + '.pb.h"', 
                CREATE_TIME = datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
                REQUEST_TYPE = each['request_type'],
                RESPONSE_TYPE = each['response_type'],
                STUBCLASS = stub_name,
                METHOD_NAME = each['method_name'],
                FILE_NAME = 'test_' + each['interface_name'] + '_client.cpp',
            )
            out_interface_cc_file = open(file, 'w')
            out_interface_cc_file.write(cc_file_content)
            out_interface_cc_file.close()
        else:
            print("file: [" + file + "] exist, skip")

    print('End generate each interface.cpp & interface.h & test_interface_client.h')
    print('=' * 100)
        
    print('End generate rocket framework code')
    print('=' * 100)



def generator_project():

    try:
        parseInput()
        print('=' * 100)
        print("Begin to generate rocket rpc server")

        generate_dir()
        generate_pb()
        if build_type == "cmake":
            generate_cmake_file()
        else:
            generate_makefile()
        
        generate_run_script()
        generate_conf_file()
        generate_framework_code()

        print("success genarate rocket project")
        print('=' * 100)

    except Exception as e:
        print("failed to generate rocket rpc server, err:" + e )
        trace.print_exc()
        print('='  * 150)
        


if __name__ == '__main__':
    generator_project()