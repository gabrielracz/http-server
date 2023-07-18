CXX      = gcc
CXXFLAGS = -g -fsanitize=address
LDFLAGS  =
LIBS= -lpthread -lm

TARGET = http-server
SRCDIR  = src
BUILDDIR = build
HDRS   = $(wildcard $(SRCDIR)/*.h) 
SRCS   = $(wildcard $(SRCDIR)/*.c) 
OBJS   = $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SRCS))
DEPS   = $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.depends,$(SRCS))

.PHONY: clean all

all: $(TARGET)

$(TARGET): $(OBJS) $(HDRS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(OBJS) -o $(TARGET) $(LIBS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c $(HDRS)
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(LIBS)

$(BUILDDIR)/%.depends: $(SRCDIR)/%.c
	$(CXX) -M $(CXXFLAGS) $< > $@

clean:
	rm -f $(OBJS) $(DEPS) $(TARGET)

-include $(DEPS)
