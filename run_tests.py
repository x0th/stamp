#!/usr/bin/env python3
import argparse
import subprocess
import sys
import os
import re
from progress.bar import Bar

obj_regex = re.compile('([A-Z][A-Za-z_]*-)([a-z0-9]+)')
blue = '\033[94m'
green = '\033[92m'
red = '\033[91m'
endc = '\033[0m'
autobake = True

def make_debug():
	subprocess.run(['make', 'clean'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
	proc = subprocess.Popen(['make', 'debug'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	_, stderr = proc.communicate()
	stderr = stderr.decode()
	if (stderr != ''):
		print(stderr, end='')
		sys.exit(1)

def make_release():
	subprocess.run(['make', 'clean'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
	proc = subprocess.Popen(['make'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	_, stderr = proc.communicate()
	stderr = stderr.decode()
	if (stderr != ''):
		print(stderr, end='')
		sys.exit(1)

def run_test(test):
	proc = subprocess.Popen(['./stamp', test], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	stdout, stderr = proc.communicate()
	return (stdout.decode(), stderr.decode())

def produce_test_out(test):
	stdout, stderr = run_test(test)
	stdout = obj_regex.sub(r'\g<1>hash', stdout)
	
	out = 'STDOUT:\n'
	out += stdout
	out += 'STDERR:\n'
	out += stderr
	return out

def bake(bake_list):
	debug_list = list(filter(lambda f: f.startswith('tests/dbg'), bake_list))
	release_list = list(filter(lambda f: not f.startswith('tests/dbg'), bake_list))
	
	if len(debug_list) != 0:
		bar = Bar('Baking debug tests.  ', max=len(debug_list))
		make_debug()
		for i in range(len(debug_list)):
			with open(debug_list[i].split('.')[0]+'.out', 'w') as fhandle:
				fhandle.write(produce_test_out(debug_list[i]))
			
			bar.next()

		bar.finish()

	if len(release_list) != 0:
		bar = Bar('Baking release tests.', max=len(release_list))
		make_release()
		for i in range(len(release_list)):
			with open(release_list[i].split('.')[0]+'.out', 'w') as fhandle:
				fhandle.write(produce_test_out(release_list[i]))

			bar.next()

		bar.finish()

	print('Done baking tests.')

def test(tests):
	debug_list = list(filter(lambda f: f.startswith('tests/dbg'), tests))
	release_list = list(filter(lambda f: not f.startswith('tests/dbg'), tests))
	failed_tests = []
	error_log = []
	
	if len(debug_list) != 0:
		bar = Bar('Running debug tests.  ', max=len(debug_list))
		make_debug()
		for i in range(len(debug_list)):
			test_out = produce_test_out(debug_list[i])

			bake_file = debug_list[i].split('.')[0]+'.out'

			if (not os.path.exists(bake_file)):
				if autobake:
					with open(bake_file, 'w') as fhandle:
						fhandle.write(test_out)
				else:
					error_log.append(blue + debug_list[i] + endc + ': No .out file found for test. Bake the test or rerun without -a to autobake.')
			else:
				with open(bake_file, 'r') as fhandle:
					expected = fhandle.read()

				if test_out != expected:
					failed_tests.append((debug_list[i], expected, test_out))

			bar.next();
	
		bar.finish()

	if len(release_list) != 0:
		bar = Bar('Running release tests.', max=len(release_list))
		make_release()
		for i in range(len(release_list)):
			test_out = produce_test_out(release_list[i])

			bake_file = release_list[i].split('.')[0]+'.out'

			if (not os.path.exists(bake_file)):
				if autobake:
					with open(bake_file, 'w') as fhandle:
						fhandle.write(test_out)
				else:
					error_log.append(blue + release_list[i] + endc + ': No .out file found for test. Bake the test or rerun without -a to autobake.')
			else:
				with open(bake_file, 'r') as fhandle:
					expected = fhandle.read()

				if test_out != expected:
					failed_tests.append((release_list[i], expected, test_out))

			bar.next();

		bar.finish()

	print()
	for e in error_log:
		print(e)
	for filename, expected, got in failed_tests:
		print(blue + filename + ':' + endc)
		print(green + 'EXPECTED:\n' + endc + expected)
		print(red + 'GOT:\n' + endc + got + '\n')

	print('\nFinished running tests. ', end='')
	print(green + 'Correct' + endc + ': ' + str(len(tests) - len(failed_tests)) + '. ', end='')
	print(red + 'Failed' + endc + ': ' + str(len(failed_tests)) + '.')

def eval_all(should_test):
	test_list = []
	for root, subdirs, files in os.walk('tests'):
		for f in files:
			if f.endswith('.st'):
				test_list.append(os.path.join(root, f))
	
	if should_test:
		test(test_list)
	else:
		bake(test_list)

if __name__ == "__main__":
	parser = argparse.ArgumentParser(description='The Stamp language interpreter.')
	parser.add_argument('-b', dest='bake_list', nargs='*', help='Store new test results.')
	parser.add_argument('-f', dest='tests', nargs='*', help='Run specified test files.')
	parser.add_argument('-a', dest='autobake', default=True, action='store_false', help='Autobake tests that do not have .out files.')

	args = parser.parse_args()

	autobake = args.autobake

	if args.bake_list is not None:
		if len(args.bake_list) == 0:
			eval_all(False)
		else:
			bake(args.bake_list)
	
	if args.tests is not None:
		if len(args.tests) == 0:
			eval_all(True)
		else:
			test(args.tests)

	if args.bake_list is None and args.tests is None:
		eval_all(True)