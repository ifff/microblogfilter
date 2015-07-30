# _*_ coding: utf-8 _*_

# read status json file 
# generate preprocessed text for each status

import datetime
import sys
import gzip
reload(sys)
sys.setdefaultencoding('utf-8')

import time
import os
from utils import extractStatus, extractOriginStatus

def parseStreamFromTime(year,month,day,streamDir,parsedDir): # start from one specific day
	cur_time = datetime.datetime(year,month,day,0)
	cur_suffix = cur_time.strftime('%Y-%m-%d-%H')
	#suffix2 = cur_time.strftime('%Y-%m-%d')
	#cur_day = cur_time.strftime('%d')
	#cur_parsed_file = open(cur_parsed_path, 'w')
	while True:
		cur_stream_path = '%s/statuses.log.%s.gz' % (streamDir,cur_suffix)
		cur_parsed_path = '%s/statuses.parsed.%s' % (parsedDir,cur_suffix)
		if os.path.exists(cur_stream_path): # current gzip file is ready
			print 'current parsed stream file is %s (ready)' % cur_stream_path
			cur_parsed_file = open(cur_parsed_path, 'w')
			with gzip.open(cur_stream_path,'rt') as cur_stream_file:
				for lineno, line in enumerate(cur_stream_file):
					out_str = extractStatus(line)
					if out_str == '': continue # skip empty tweet after stemmered, stopword removal
					origin_str = extractOriginStatus(line)
					out_str = out_str + '\t' + origin_str + '\n'
					cur_parsed_file.write(out_str)
			cur_stream_file.close()
			cur_parsed_file.close()
			cur_time = cur_time + datetime.timedelta(hours=1) # change to next hour
			# check whether day changes
			#new_day = cur_time.strftime('%d')
			#if new_day != cur_day:break
			cur_suffix = cur_time.strftime('%Y-%m-%d-%H')
		else:
			# sleep to wait
			print 'current parsed stream file is %s (not ready), sleep to wait...' % cur_stream_path
			time.sleep(30)
			# # do some thing, try to add one hour
			# print 'Try add one hour...'
			# cur_time = cur_time + datetime.timedelta(hours=1)
			# # check whether day changes
			# new_day = cur_time.strftime('%d')
			# if new_day != cur_day:break
			# suffix1 = cur_time.strftime('%Y-%m-%d-%H')
			# print 'current parsed stream file is %s...' % suffix1
			# cur_stream_path = '../statuses.log.%s.gz' % suffix1 # next hour

def parseCurStream():
	cur_time = datetime.datetime.now()
	suffix1 = cur_time.strftime('%Y-%m-%d-%H')
	suffix2 = cur_time.strftime('%Y-%m-%d-%H')
	cur_day = cur_time.strftime('%d')
	print 'current parsed stream is %s' % suffix1
	cur_stream_path = '../statuses.log.%s' % suffix1
	cur_parsed_path = './parsed/statuses.parsed.%s' % suffix2
	#cur_parsed_file = open(cur_parsed_path, 'w')
	
	# next hour stream
	next_time = cur_time + datetime.timedelta(hours=1)
	next_suffix = next_time.strftime('%Y-%m-%d-%H')
	next_stream_path = '../statuses.log.%s' % next_suffix
	
	while True:
		if os.path.exists(cur_stream_path):
			cur_stream_file = open(cur_stream_path, 'r')
			cur_parsed_file = open(cur_parsed_path, 'w')
			while True:
				line = cur_stream_file.readline()
				if not line and os.exists(next_stream_path): # change to next hour
					break
				out_str = extractStatus(line)
				if out_str == '': continue # skip empty tweet after stemmered, stopword removal
				origin_str = extractOriginStatus(line)
				out_str = out_str + '\t' + origin_str + '\n'
				cur_parsed_file.write(out_str)
			cur_stream_file.close()
			cur_parsed_file.close()
			# add one hour, update time
			cur_time = datetime.datetime.now()
			suffix1 = cur_time.strftime('%Y-%m-%d-%H')
			cur_stream_path = '../statuses.log.%s' % suffix1
			print 'current parsed stream is %s' % suffix1
			next_time = cur_time + datetime.timedelta(hours=1)
			next_suffix = next_time.strftime('%Y-%m-%d-%H')
			next_stream_path = '../statuses.log.%s' % next_suffix
			# check whether day changes
			new_day = cur_time.strftime('%d')
			if new_day != cur_day:
				cur_parsed_file.close()
				suffix2 = cur_time.strftime('%Y-%m-%d')
				cur_parsed_path = './parsed/statuses.parsed.%s' % suffix2
				cur_parsed_file = open(cur_parsed_path, 'w')
			
# @param raw stream dir
# @param parsed parsed dir
def startListenStream(year,month,day,streamDir,parsedDir):
	#parseCurStream()
	parseStreamFromTime(year,month,day,streamDir,parsedDir)
	# start_time = datetime.datetime(2015,7,1,10)

	#print start_time
	# current_time = start_time
	# while True:
		# t1 = datetime.datetime.now()
		# file_suffix = current_time.strftime('%Y-%m-%d-%H')
		# cur_path = streamDir + '../statuses.log.%s.gz' % file_suffix	
		# cur_parsed_path = parsedDir + '/statuses.parsed.%s' % file_suffix
		# if os.path.exists(path):
			# current_parsed_file = open(cur_parsed_path, 'w')
			# with gzip.open(path,'rt') as current_status_file:
				# for lineno, line in enumerate(current_status_file):
					# if (lineno + 1) % 100 == 0:
						# print 'file %s %d lines preprocessed' % (file_suffix, lineno)
					# out_str = extractStatus(line)
					# if out_str == '':continue
					# current_parsed_file.write(out_str)

			# current_status_file.close()
			# current_parsed_file.close()

			#start read next stream file hourly	
			# current_time = current_time + datetime.timedelta(hours=1)
			# t2 = datetime.datetime.now()
			# print 'Time:',(t2-t1).seconds
			# print str(current_time)
			# break
			

if __name__ == "__main__":
	if len(sys.argv) < 4:
		print 'argv[1]:year'
		print 'argv[2]: month'
		print 'argv[3]: day'
		print 'argv[4]:streamDir'
		print 'argv[5]: parsedDir'
		exit()
	year = int(sys.argv[1])
	month = int(sys.argv[2])
	day = int(sys.argv[3])

	streamDir = '../'
	parsedDir = './parsed/'
	startListenStream(year,month,day,streamDir,parsedDir)
