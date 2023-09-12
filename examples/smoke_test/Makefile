ifdef PKG_CONFIG_TEST_ARGS
  CPPFLAGS = $(shell pkg-config $${PKG_CONFIG_TEST_ARGS} --cflags vtkm)
  LDLIBS = $(shell pkg-config $${PKG_CONFIG_TEST_ARGS} --libs vtkm)

else ifdef VTKM_CONFIG_MK_PATH
  include $(VTKM_CONFIG_MK_PATH)
  VTKm_DIR = $(VTKM_INSTALL_PREFIX)
  CPPFLAGS = $(VTKm_INCLUDE_FLAGS)
  LDLIBS = $(VTKm_LIB_FLAGS)

endif

all: smoke_test

check: smoke_test
	./smoke_test

%.o : %.cxx
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $^

smoke_test.o: smoke_test.cxx

smoke_test: CC=$(CXX)
smoke_test: smoke_test.o
