#!/usr/bin/env python

import sys
import getopt
import struct


BLOCKS      = 0x1
GEOMETRY    = 0x2
SCENE       = 0x4
ANIMATION   = 0x8

include = 0
offset = 0
indent_level = 0


def printl(str=''):
    global indent_level

    pad = ' ' * indent_level * 2
    print('%s%s' % (pad, str))



def print_header(data):
    compressions = ('uncompressed', 'deflate (file-level)', 'lzma (file-level)')
    header = struct.unpack_from('>BBHBI', data, 3)

    if header[3] < len(compressions):
        compression = compressions[header[3]]
    else:
        compression = '<error> %s' % hex(header[3])

    printl('version:      %d.%d' % (header[0], header[1]))
    printl('compression:  %s' % compression)
    printl('body size:    %d (%s)' % (header[4], hex(header[4])))
    printl()

    return 12

def read_var_str(data, offs=0):
    len = struct.unpack_from('>H', data, offs)
    str = struct.unpack_from('%ds' % len[0], data, offs+2)

    return str[0]

def print_properties(data):
    global indent_level

    offs = 0

    printl()
    props_len = struct.unpack_from('>I', data, offs)[0]
    offs += 4
    if props_len > 0:
        printl('PROPERTIES: (%db)' % props_len)
        props_end = offs + props_len

        indent_level += 1
        while (offs < props_end):
            prop_key, prop_len = struct.unpack_from('>HH', data, offs)
            offs += 4
            prop_end = offs + prop_len
            val_str = ''
            while (offs < prop_end):
                val_str += '%02x ' % struct.unpack_from('>B', data, offs)[0]
                offs += 1

            printl('%d: %s' % (prop_key, val_str))

        indent_level -= 1

    return offs

def print_user_attributes(data):
    global indent_level

    offs = 0
    attr_len = struct.unpack_from('>I', data, offs)[0]
    offs += 4

    if attr_len > 0:
        printl('USER ATTRIBUTES (%db)' % attr_len)

    return offs
                


def print_skeleton(data):
    global indent_level

    name = read_var_str(data)
    offs = 2+len(name)

    num_joints = struct.unpack_from('>I', data, offs)[0]
    offs += 4

    printl('NAME: %s' % name)
    printl('JOINTS: %d' % num_joints)

    offs += print_properties(data[offs:])

    indent_level += 1
    joints_printed = 0
    while offs < len(data) and joints_printed < num_joints:
        joint_id, parent_id = struct.unpack_from('>II', data, offs)
        offs += 8

        joint_name = read_var_str(data, offs)
        printl('JOINT %s (id=%d, parent=%d)' % (
            joint_name, joint_id, parent_id))
            
        offs += (2 + len(joint_name) + 128)
        joints_printed += 1

    indent_level -= 1

    offs += print_user_attributes(data[offs:])
        


def print_mesh_instance(data):
    global indent_level

    parent = struct.unpack_from('>I', data)[0]
    matrix = struct.unpack_from('>16d', data, 4)
    data_id = struct.unpack_from('>I', data, 132)[0]

    printl('DATA ID: %d' % data_id)
    printl('PARENT ID: %d' % parent)
    printl('TRANSFORM MATRIX:')
    for i in range(0, 15, 4):
        printl('%f %f %f %f' % (matrix[i], matrix[i+1], matrix[i+2], matrix[i+3]))


def print_mesh_data(data):
    global indent_level

    name = read_var_str(data)
    offs = (2 + len(name)) # var str 
    num_subs = struct.unpack_from('>H', data, offs)[0]
    offs += 2

    printl('NAME: %s' % name)
    printl('SUB-MESHES: %d' % num_subs)

    offs += print_properties(data[offs:])

    printl()

    subs_printed = 0
    indent_level += 1
    while offs < len(data) and subs_printed < num_subs:
        mat_id = struct.unpack_from('>I', data, offs)[0]
        length = struct.unpack_from('>I', data, offs+4)[0]
        offs += 8

        printl('SUB-MESH')
        indent_level += 1
        printl('Material ID: %d' % mat_id)
        printl('Length:      %d' % length)
        indent_level -= 1

        sub_end = offs + length

        indent_level += 1
        while offs < sub_end:
            stream_types = ('', 'VERTEX', 'TRIANGLE', 'UV', '', '', '', 'WEIGHTS')
            type, str_len = struct.unpack_from('>BI', data, offs)
            offs += 5

            if type < len(stream_types):
                stream_type = stream_types[type]
                if type == 1 or type == 3 or type==7:
                    elem_data_format = 'f'
                    elem_print_format = '%f'
                elif type == 2:
                    elem_data_format = 'H'
                    elem_print_format = '%d'
            else:
                stream_type = '<error>'
            
            printl('STREAM (%s)' % stream_type)
            indent_level += 1
            printl('Length: %d' % str_len)

            str_end = offs + str_len
            while offs < str_end:
                element = struct.unpack_from('>%s' % elem_data_format, data, offs)
                printl(elem_print_format % element[0])
                offs += struct.calcsize(elem_data_format)

            printl()
            indent_level -= 1
                
        subs_printed += 1
        indent_level -= 1

    indent_level -= 1

    offs += print_user_attributes(data[offs:])


def print_next_block(data):
    global indent_level

    block_types = {
        '3':    'MeshInst',
        '4':    'MeshData',
        '60':   'Skeleton',
    }

    block_header = struct.unpack_from('>IBBI', data, offset)

    type = block_header[2]
    length = block_header[3]

    if str(type) in block_types:
        block_type = block_types[str(type)]
    else:
        block_type = '<error> %s' % hex(type)

    printl('BLOCK %s' % block_type)
    indent_level += 1
    printl('NS: %d, ID: %d' % (block_header[1], block_header[0]))
    printl('Length: %d' % length)

    if type == 3 and include&SCENE:
        printl()
        print_mesh_instance(data[offset+10 : offset+10+length])
    elif type == 4 and include&GEOMETRY:
        printl()
        print_mesh_data(data[offset+10 : offset+10+length])
    elif type == 60 and include&ANIMATION:
        printl()
        print_skeleton(data[offset+10 : offset+10+length])

    printl()
    indent_level -= 1
    return 10 + length


if __name__ == '__main__':
    opts, files = getopt.getopt(sys.argv[1:], 'bgsax')

    for opt in opts:
        if opt[0] == '-b':
            include |= BLOCKS
        elif opt[0] == '-g':
            include |= (GEOMETRY | BLOCKS)
        elif opt[0] == '-s':
            include |= (SCENE | BLOCKS)
        elif opt[0] == '-a':
            include |= (ANIMATION | BLOCKS)
        elif opt[0] == '-x':
            include = 0xffff
            

    for file in files:
        f = open(file, 'rb')
        data = f.read()
        printl(file)

        indent_level += 1
        offset = print_header(data)

        if include & BLOCKS:
            while offset < len(data):
                offset += print_next_block(data)
