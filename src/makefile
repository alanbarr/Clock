CC=msp430-gcc
CCFLAGS=-Wall -g -Os

SZ=msp430-size

OUTPUT=vfd_clock.elf
OUTPUTDIR=output

TARGET=msp430g2553

DEPS=$(OUTPUTDIR)/.depend

VPATH=$(OUTPUTDIR)

SRCS= main.c    \
      vfd.c     \
      bcd_rtc.c \
      alarm.c   \
      temperature.c

OBJS=${SRCS:.c=.o}
PPDS=${SRCS:.c=.E} 
ASMS=${SRCS:.c=.S}

all: $(OUTPUT)

preprocess: $(OUTPUTDIR) $(PPDS)

compile: $(OUTPUTDIR) $(ASMS)

$(DEPS): $(OUTPUTDIR) $(SRCS)
	-rm $(DEPS)
	$(CC) $(CFLAGS) $(CPPFLAGS) -MM $(SRCS) > $(DEPS)

include $(DEPS)

$(OUTPUT): $(OBJS)
	$(CC) $(CCFLAGS) $(CPPFLAGS) -mmcu=$(TARGET) -o $(OUTPUTDIR)/$(OUTPUT) ${addprefix output/,$(OBJS)}
	$(SZ) $(OUTPUTDIR)/$(OUTPUT)

%.o: %.c
	$(CC) $(CCFLAGS) $(CPPFLAGS) -mmcu=$(TARGET) -o $(OUTPUTDIR)/$@ -c $<

%.E: %.c
	$(CC) $(CCFLAGS) $(CPPFLAGS) -mmcu=$(TARGET) -o $(OUTPUTDIR)/$@ -E $< 

%.S: %.c
	$(CC) $(CCFLAGS) $(CPPFLAGS) -mmcu=$(TARGET) -o $(OUTPUTDIR)/$@ -S $<

$(OUTPUTDIR):
	test -d $(OUTPUTDIR) || mkdir $(OUTPUTDIR)

clean:
	-rm $(DEPS)
	-rm $(OUTPUTDIR)/*
	-rmdir $(OUTPUTDIR)
	

