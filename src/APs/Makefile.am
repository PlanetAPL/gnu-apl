
bin_PROGRAMS = AP100 AP210 APnnn

common_files = APmain.cc APmain.hh	\
		../CDR_string.hh ../Svar_signals.hh	\
                ../UdpSocket.cc ../UdpSocket.hh		\
		../Svar_DB.cc ../Svar_DB.hh		\
                ../Backtrace.cc ../Backtrace.hh

AP100_SOURCES = AP100.cc $(common_files)
AP210_SOURCES = AP210.cc $(common_files)
APnnn_SOURCES = APnnn.cc $(common_files)

AP100_CXXFLAGS = -I .. -g2 -DAP_NUM=100
AP210_CXXFLAGS = -I .. -g2 -DAP_NUM=210
APnnn_CXXFLAGS = -I .. -g2 -DAP_NUM=0

AM_MAKEFLAGS = -j 2

