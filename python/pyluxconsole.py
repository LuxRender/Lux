#!/usr/bin/env python3
# -*- coding: utf8 -*-
#
# ***** BEGIN GPL LICENSE BLOCK *****
#
# Authors:
# Doug Hammond
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see <http://www.gnu.org/licenses/>.
#
# ***** END GPL LICENCE BLOCK *****
#
#------------------------------------------------------------------------------ 
# System imports
#------------------------------------------------------------------------------ 
import optparse, datetime, os, sys, threading, time

#------------------------------------------------------------------------------ 
# Lux imports
#------------------------------------------------------------------------------ 
try:
	import pylux
except ImportError as err:
	print('This script requires pylux: %s'%err)
	sys.exit()

#------------------------------------------------------------------------------
# Class Definitions
#------------------------------------------------------------------------------ 

class TimerThread(threading.Thread):
	'''
	Periodically call self.kick()
	'''
	STARTUP_DELAY = 0
	KICK_PERIOD = 8
	
	active = True
	timer = None
	
	LocalStorage = None
	
	def __init__(self, LocalStorage=dict()):
		threading.Thread.__init__(self)
		self.LocalStorage = LocalStorage
	
	def set_kick_period(self, period):
		self.KICK_PERIOD = period + self.STARTUP_DELAY
	
	def stop(self):
		self.active = False
		if self.timer is not None:
			self.timer.cancel()
			
	def run(self):
		'''
		Timed Thread loop
		'''
		
		while self.active:
			self.timer = threading.Timer(self.KICK_PERIOD, self.kick_caller)
			self.timer.start()
			if self.timer.isAlive(): self.timer.join()
	
	def kick_caller(self):
		if self.STARTUP_DELAY > 0:
			self.KICK_PERIOD -= self.STARTUP_DELAY
			self.STARTUP_DELAY = 0
		
		self.kick()
	
	def kick(self):
		'''
		sub-classes do their work here
		'''
		pass

def format_elapsed_time(t):
	'''
	Format a duration in seconds as an HH:MM:SS format time
	'''
	
	td = datetime.timedelta(seconds=t)
	min = td.days*1440  + td.seconds/60.0
	hrs = td.days*24	+ td.seconds/3600.0
	
	return '%i:%02i:%02i Elapsed' % (hrs, min%60, td.seconds%60)

class LuxAPIStats(TimerThread):
	'''
	Periodically get lux stats
	'''
	
	KICK_PERIOD = 0.5
	
	stats_order = [
		'secElapsed',
		'samplesSec',
		'samplesTotSec',
		'samplesPx',
		'efficiency',
	]
	
	stats_dict = {
		'secElapsed':		0.0,
		'samplesSec':		0.0,
		'samplesTotSec':	0.0,
		'samplesPx':		0.0,
		'efficiency':		0.0,
		#'filmXres':		0.0,
		#'filmYres':		0.0,
		#'displayInterval':	0.0,
		#'filmEV':			0.0,
		#'sceneIsReady':	0.0,
		#'filmIsReady':		0.0,
		#'terminated':		0.0,
		#'enoughSamples':	0.0,
	}
	
	stats_format = {
		'secElapsed':		format_elapsed_time,
		'samplesSec':		lambda x: 'Samples/Sec: %0.2f'%x,
		'samplesTotSec':	lambda x: 'Total Samples/Sec: %0.2f'%x,
		'samplesPx':		lambda x: 'Samples/Px: %0.2f'%x,
		'efficiency':		lambda x: 'Efficiency: %0.2f %%'%x,
		#'filmEV':			lambda x: 'EV: %0.2f'%x,
		#'sceneIsReady':	lambda x: 'SIR: '+ ('True' if x else 'False'),
		#'filmIsReady':		lambda x: 'FIR: %f'%x,
		#'terminated':		lambda x: 'TERM: %f'%x,
		#'enoughSamples':	lambda x: 'HALT: '+ ('True' if x else 'False'),
	}
	
	stats_string = ''
	
	def stop(self):
		self.active = False
		if self.timer is not None:
			self.timer.cancel()
			
	def kick(self):
		for k in self.stats_dict.keys():
			self.stats_dict[k] = self.LocalStorage['lux_context'].statistics(k)
		
		self.stats_string = ' - '.join(['%s'%self.stats_format[k](self.stats_dict[k]) for k in self.stats_order])
		
		network_servers = self.LocalStorage['lux_context'].getServerCount()
		if network_servers > 0:
			self.stats_string += ' - %i Network Servers Active' % network_servers

def log(str, module_name='Lux'):
	print("[%s %s] %s" % (module_name, time.strftime('%Y-%b-%d %H:%M:%S'), str))

if __name__ == '__main__':
	# Set up command line options
	parser = optparse.OptionParser(
		description = 'LuxRender console/command line interface',
		usage = 'usage: %prog [options] <scene file> [<scene file> ...]',
		version = '%%prog version %s'%pylux.version()
	)
	# Generic
	parser.add_option(
		'-V', '--verbose',
		action = 'store_true', default = False,
		dest = 'verbose',
		help = 'Increase output verbosity (show DEBUG messages)'
	)
	parser.add_option(
		'-q', '--quiet',
		action = 'store_true', default = False,
		dest = 'quiet',
		help = 'Reduce output verbosity (hide INFO messages)'
	)
	parser.add_option(
		'-d', '--debug',
		action = 'store_true', default = False,
		dest = 'debug',
		help = 'Enable debug mode'
	)
	parser.add_option(
		'-f', '--fixedseed',
		action = 'store_true', default = False,
		dest = 'fixedseed',
		help = 'Disable random seed mode'
	)
	
	# TODO add Epsilon settings
	
	# Server mode
	parser.add_option(
		'-s', '--server',
		action = 'store_true', default = False,
		dest = 'server',
		help = 'Launch in server mode'
	)
	parser.add_option(
		'-p', '--serverport',
		metavar = 'P', type = int,
		dest = 'serverport',
		help = 'Specify the tcp port used in server mode'
	)
	parser.add_option(
		'-W', '--serverwriteflm',
		action = 'store_true', default = False,
		dest = 'serverwriteflm',
		help = 'Write film to disk before transmitting in server mode'
	)
	# Client mode
	parser.add_option(
		'-u', '--useserver',
		metavar = 'ADDR', type = str,
		action = 'append',
		dest = 'useserver',
		help = 'Specify the address of a rendering server to use'
	)
	parser.add_option(
		'-i', '--serverinterval',
		metavar = 'S', type = int,
		dest = 'serverinterval',
		help = 'Specify the number of seconds between requests to rendering servers'
	)
	# Rendering options
	parser.add_option(
		'-t', '--threads',
		metavar = 'T', type = int,
		dest = 'threads',
		help = 'Number of rendering threads'
	)
	
	
	(options, args) = parser.parse_args()
	
	if len(sys.argv) < 2:
		parser.print_help()
		sys.exit()
	
	# luxErrorFilter is not exposed in pylux yet, so verbose and quiet cannot be set
	if options.verbose or options.quiet:
		print('--verbose and --quiet options are not yet implemented, ignoring.')
	
	if options.server:
		print('--server mode not yet implemented, ignoring.')
	
	#print(options)
	#print(args)
	
	if len(args) < 1:
		log('No files to render!')
		sys.exit()
	
	ctx = pylux.Context('pyluxconsole')
	
	if options.threads is None:
		threads = 0
	else:
		threads = options.threads - 1
	
	if options.debug:
		ctx.enableDebugMode()
	
	if options.fixedseed and not options.server:
		ctx.disableRandomMode()
	
	if options.serverinterval is not None:
		ctx.setNetworkServerUpdateInterval(options.serverinterval)
	
	for scene_file in args:
		if not os.path.exists(scene_file):
			log('Scene file to render "%s" does not exist, skipping.'%scene_file)
			continue
		
		if options.useserver is not None:
			for srv in options.useserver:
				ctx.addServer(srv)
		
		os.chdir(os.path.dirname(scene_file))
		
		stats_thread = LuxAPIStats({
			'lux_context': ctx
		})
		
		ctx.parse(scene_file, True) # asynchronous parse (ie. don't wait)
		
		stats_thread.start()
		
		# wait here for parsing to complete
		while ctx.statistics('sceneIsReady') != 1.0:
			time.sleep(0.3)
		
		# TODO: add support to pylux for reporting parse errors after async parse
		
		for i in range(threads):
			ctx.addThread()
		
		while ctx.statistics('filmIsReady') != 1.0 and \
			  ctx.statistics('terminated') != 1.0 and \
			  ctx.statistics('enoughSamples') != 1.0:
			try:
				time.sleep(5)
				log(stats_thread.stats_string)
			except KeyboardInterrupt:
				log('Stopping render...')
				break
		
		ctx.exit()
		ctx.wait()
		
		stats_thread.stop()
		stats_thread.join()
		
		ctx.cleanup()
		
		if options.useserver is not None:
			for srv in options.useserver:
				ctx.removeServer(srv)
	