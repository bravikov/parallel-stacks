import subprocess
import re


class Frame:
    def __init__(self, level, address, function, parameters, filename):
        self.level = level
        self.address = address
        self.function = function
        self.parameters = parameters
        self.filename = filename

    def __str__(self):
        return "level: {}, address: '{}', function: '{}', parameters: '{}', filename: '{}'".format(
            self.level,
            self.address,
            self.function,
            self.parameters,
            self.filename,
        )


class Thread:
    def __init__(self, thread_id):
        self.id = thread_id
        self.frames = []

    def __str__(self):
        result = 'Thread {}'.format(self.id)
        for frame in self.frames:
            result += '\n  ' + str(frame)
        return result


def parse_gdb_output(gdb_output):
    threads = []
    current_thread = None

    for gdb_output_line in gdb_output.splitlines():
        if gdb_output_line.startswith('Thread '):
            thread_info = gdb_output_line.split()
            thread_id = thread_info[1]
            current_thread = Thread(thread_id)
            threads.append(current_thread)
            continue
        elif current_thread:
            if len(gdb_output_line) == 0:
                current_thread = None
                continue
            match_obj = re.match('#(?P<level>\\d+) +((?P<address>0x[\\da-f]+) in |)(?P<function>.+) \\((?P<parameters>.*)\\)( (at|from) (?P<filename>.+)|)', gdb_output_line)
            if match_obj:
                frame = Frame(
                    level=int(match_obj.group('level')),
                    address=match_obj.group('address'),
                    function=match_obj.group('function'),
                    parameters=match_obj.group('parameters'),
                    filename=match_obj.group('filename')
                )
                current_thread.frames.append(frame)
            else:
                raise SyntaxError("gdb output: '{}'".format(gdb_output_line))

    return threads


class Node:
    def __init__(self):
        self.function = None
        self.nodes = []
        self.depth = 0

    def __str__(self):
        result = ''
        if self.function:
            result += '{}{}'.format('  ' * self.depth, self.function)
        result += '\n'
        for node in self.nodes:
            result += str(node)
        return result


def get_parallel_stacks(threads, current_function=None, depth=0):
    node = Node()
    node.depth = depth
    node.function = current_function
    function_threads = {}
    level = -depth - 1
    for thread in threads:
        if depth >= len(thread.frames):
            continue
        function = thread.frames[level].function
        threads = function_threads.get(function, None)
        if threads:
            threads.append(thread)
        else:
            function_threads[function] = [thread]

    for function in function_threads:
        child_node = get_parallel_stacks(function_threads[function], function, depth + 1)
        node.nodes.append(child_node)
    return node


def main():
    process_id = 13552
    gdb_command = ['gdb', '--batch', '-ex', 'thread apply all bt', '-pid', str(process_id)]
    gdb_result = subprocess.run(gdb_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    gdb_output = gdb_result.stdout.decode('utf-8')
    threads = parse_gdb_output(gdb_output)
    for thread in threads:
        print(thread)

    tree = get_parallel_stacks(threads)
    print(tree)


main()
