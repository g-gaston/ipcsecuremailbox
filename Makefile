# Master Makefile to compile everything in /usr/src except the system.

MAKE=make

usage:
	@echo "" 
	@echo "Master Makefile for MINIX commands and utilities." 
	@echo "Root privileges are required for some actions." 
	@echo "" 
	@echo "Usage:" 
	@echo "	make world      # Compile everything (libraries & commands)" 
	@echo "	make includes   # Install include files from src/" 
	@echo "	make libraries  # Compile and install libraries" 
	@echo "	make commands   # Compile all, commands, but don't install"
	@echo "	make install    # Compile and install commands" 
	@echo "	make depend     # Generate required .depend files" 
	@echo "	make clean      # Remove all compiler results" 
	@echo "" 
	@echo "Run 'make' in tools/ to create a new MINIX configuration." 
	@echo "" 

# world has to be able to make a new system, even if there
# is no complete old system. it has to install commands, for which
# it has to install libraries, for which it has to install includes,
# for which it has to install /etc (for users and ownerships).
# etcfiles also creates a directory hierarchy in its
# 'make install' target.
# 
# etcfiles has to be done first.
world: includes depend libraries install postinstall

includes:
	cd include && $(MAKE) install gcc

libraries:
	cd lib && sh ack_build.sh obj depend all install

gnu-libraries:
	cd lib && sh gnu_build.sh obj depend all install

commands:
	cd commands && $(MAKE) all

depend::
	mkdep kernel
	mkdep servers
	mkdep drivers
	cd kernel && $(MAKE) $@
	cd servers && $(MAKE) $@
	cd drivers && $(MAKE) $@

etcfiles::
	cd etc && $(MAKE) install

all::
	cd boot && $(MAKE) all
	cd man && $(MAKE) all
	cd commands && $(MAKE) all
	cd tools && $(MAKE) all
	cd servers && $(MAKE) all

install::
	cd boot && $(MAKE) all install
	cd man && $(MAKE) all install
	cd commands && $(MAKE) all install
	cd tools && $(MAKE) all install
	cd servers && $(MAKE) all install

clean::
	cd boot && $(MAKE) clean
	cd man && $(MAKE) clean
	cd commands && $(MAKE) clean
	cd tools && $(MAKE) clean
	cd servers && $(MAKE) clean
	cd lib && sh ack_build.sh clean
	cd lib && sh gnu_build.sh clean
	cd commands && $(MAKE) clean
	cd test && $(MAKE) clean

postinstall:
	cd etc && $(MAKE) $@
