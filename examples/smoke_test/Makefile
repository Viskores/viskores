ifdef PKG_CONFIG_TEST_ARGS
  CPPFLAGS = $(shell pkg-config $${PKG_CONFIG_TEST_ARGS} --cflags viskores)
  LDLIBS = $(shell pkg-config $${PKG_CONFIG_TEST_ARGS} --libs viskores)

else ifdef VISKORES_CONFIG_MK_PATH
  include $(VISKORES_CONFIG_MK_PATH)
  Viskores_DIR = $(VISKORES_INSTALL_PREFIX)
  CPPFLAGS = $(Viskores_INCLUDE_FLAGS)
  LDLIBS = $(Viskores_LIB_FLAGS)

endif

all: smoke_test

check: smoke_test
	./smoke_test

%.o : %.cxx
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $^

smoke_test.o: smoke_test.cxx

smoke_test: CC=$(CXX)
smoke_test: smoke_test.o
