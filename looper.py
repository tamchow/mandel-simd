import shutil
import argparse
import os

def copy_file_multi(prefix, suffix, forward_times, forward_start_index, forward_end_index, reverse_times, reverse_start_index, reverse_end_index):
	filelist = [filename for filename in os.listdir(os.getcwd()) if filename.startswith(suffix) and filename.endswith(suffix)]
	directory_path = str(os.getcwd())
	numbering_start = len(filelist)
	for i in range(0, forward_times):
		for j in range(forward_start_index, forward_end_index + 1):
			new_filename = prefix + str(numbering_start + (i * forward_times) + j) + suffix
			shutil.copy2("/".join(directory_path, filelist[j]), "/".join(directory_path, new_filename))
	for i in range(0, reverse_times):
		for j in range(reverse_start_index, reverse_end_index + 1):
			new_filename = prefix + str(numbering_start + (i * forward_times) + j) + suffix
			shutil.copy2("/".join(directory_path, filelist[reverse_start_index + (reverse_end_index - j)]), "/".join(directory_path, new_filename))


def main():
	parser = argparse.ArgumentParser()
	parser.add_argument("-p", "--prefix", "prefix", help="The prefix before the number in a file name")
	parser.add_argument("-s", "--suffix", "suffix", help="The suffix after the number in a file name")
	parser.add_argument("-fn", "--forward_times", "forward_times", type=int, help="The number of times to copy the input file range in input order", default = 0)
	parser.add_argument("-fsi", "--forward_start_index", "forward_start_index", type=int, help="The file number to start forward copying at", default=0)
	parser.add_argument("-fei", "--forward_end_index", "forward_end_index", type=int, help="The file number to end forward copying at", default=0)
	parser.add_argument("-rn", "--reverse_times", "reverse_times", type=int, help="The number of times to copy the input file range in reverse of input order", default=0)
	parser.add_argument("-rsi", "--reverse_start_index", "reverse_start_index", type=int, help="The file number to start reverse copying at", default=0)
	parser.add_argument("-rei", "--reverse_end_index", "reverse_end_index", type=int, help="The file number to end reverse copying at", default=0)
	args = parser.parse_args()
	if(args.forward_times <= 0 and args.reverse_times <= 0):
		print("Nothing to do!")
	else:
		copy_file_multi(prefix, suffix, forward_times, forward_start_index, forward_end_index, reverse_times, reverse_start_index, reverse_end_index)

if __name__ == '__main__':
	main()
