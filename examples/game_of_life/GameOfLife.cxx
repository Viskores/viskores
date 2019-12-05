//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
// Must be included before any other GL includes:
#include <GL/glew.h>

// Must be included before any other GL includes:
#include <GL/glew.h>

#include <algorithm>
#include <iostream>
#include <random>

#include <vtkm/Math.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/Initialize.h>
#include <vtkm/cont/Timer.h>

#include <vtkm/interop/TransferToOpenGL.h>

#include <vtkm/filter/FilterDataSet.h>
#include <vtkm/worklet/WorkletPointNeighborhood.h>

#include <vtkm/cont/TryExecute.h>
#include <vtkm/cont/cuda/DeviceAdapterCuda.h>
#include <vtkm/cont/serial/DeviceAdapterSerial.h>
#include <vtkm/cont/tbb/DeviceAdapterTBB.h>

//Suppress warnings about glut being deprecated on OSX
#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

//OpenGL Graphics includes
//glew needs to go before glut
//that is why this is after the TransferToOpenGL include
#if defined(__APPLE__)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include "LoadShaders.h"

struct GameOfLifePolicy : public vtkm::filter::PolicyBase<GameOfLifePolicy>
{
  using FieldTypeList = vtkm::List<vtkm::UInt8, vtkm::Vec4ui_8>;
};

struct UpdateLifeState : public vtkm::worklet::WorkletPointNeighborhood
{
  using CountingHandle = vtkm::cont::ArrayHandleCounting<vtkm::Id>;

  using ControlSignature = void(CellSetIn,
                                FieldInNeighborhood prevstate,
                                FieldOut state,
                                FieldOut color);

  using ExecutionSignature = void(_2, _3, _4);

  template <typename NeighIn>
  VTKM_EXEC void operator()(const NeighIn& prevstate,
                            vtkm::UInt8& state,
                            vtkm::Vec4ui_8& color) const
  {
    // Any live cell with fewer than two live neighbors dies, as if caused by under-population.
    // Any live cell with two or three live neighbors lives on to the next generation.
    // Any live cell with more than three live neighbors dies, as if by overcrowding.
    // Any dead cell with exactly three live neighbors becomes a live cell, as if by reproduction.
    auto current = prevstate.Get(0, 0, 0);
    auto count = prevstate.Get(-1, -1, 0) + prevstate.Get(-1, 0, 0) + prevstate.Get(-1, 1, 0) +
      prevstate.Get(0, -1, 0) + prevstate.Get(0, 1, 0) + prevstate.Get(1, -1, 0) +
      prevstate.Get(1, 0, 0) + prevstate.Get(1, 1, 0);

    if (current == 1 && (count == 2 || count == 3))
    {
      state = 1;
    }
    else if (current == 0 && count == 3)
    {
      state = 1;
    }
    else
    {
      state = 0;
    }

    color[0] = 0;
    color[1] = static_cast<vtkm::UInt8>(state * (100 + (count * 32)));
    color[2] = (state && !current) ? static_cast<vtkm::UInt8>(100 + (count * 32)) : 0;
    color[3] = 255; //alpha channel
  }
};


class GameOfLife : public vtkm::filter::FilterDataSet<GameOfLife>
{
public:
  template <typename Policy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                          vtkm::filter::PolicyBase<Policy> policy)

  {
    vtkm::cont::ArrayHandle<vtkm::UInt8> state;
    vtkm::cont::ArrayHandle<vtkm::UInt8> prevstate;
    vtkm::cont::ArrayHandle<vtkm::Vec4ui_8> colors;

    //get the coordinate system we are using for the 2D area
    const vtkm::cont::DynamicCellSet& cells = input.GetCellSet();

    //get the previous state of the game
    input.GetField("state", vtkm::cont::Field::Association::POINTS).GetData().CopyTo(prevstate);

    //Update the game state
    this->Invoke(
      UpdateLifeState{}, vtkm::filter::ApplyPolicyCellSet(cells, policy), prevstate, state, colors);

    //save the results
    vtkm::cont::DataSet output;
    output.CopyStructure(input);

    output.AddField(vtkm::cont::make_FieldPoint("colors", colors));
    output.AddField(vtkm::cont::make_FieldPoint("state", state));
    return output;
  }

  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT bool DoMapField(vtkm::cont::DataSet&,
                            const vtkm::cont::ArrayHandle<T, StorageType>&,
                            const vtkm::filter::FieldMetadata&,
                            vtkm::filter::PolicyBase<DerivedPolicy>)
  {
    return false;
  }
};

struct UploadData
{
  vtkm::interop::BufferState* ColorState;
  vtkm::cont::Field Colors;

  UploadData(vtkm::interop::BufferState* cs, vtkm::cont::Field colors)
    : ColorState(cs)
    , Colors(colors)
  {
  }
  template <typename DeviceAdapterTag>
  bool operator()(DeviceAdapterTag device)
  {
    vtkm::cont::ArrayHandle<vtkm::Vec4ui_8> colors;
    this->Colors.GetData().CopyTo(colors);
    vtkm::interop::TransferToOpenGL(colors, *this->ColorState, device);
    return true;
  }
};

struct RenderGameOfLife
{
  vtkm::Int32 ScreenWidth;
  vtkm::Int32 ScreenHeight;
  GLuint ShaderProgramId;
  GLuint VAOId;
  vtkm::interop::BufferState VBOState;
  vtkm::interop::BufferState ColorState;

  RenderGameOfLife(vtkm::Int32 width, vtkm::Int32 height, vtkm::Int32 x, vtkm::Int32 y)
    : ScreenWidth(width)
    , ScreenHeight(height)
    , ShaderProgramId()
    , VAOId()
    , ColorState()
  {
    this->ShaderProgramId = LoadShaders();
    glUseProgram(this->ShaderProgramId);

    glGenVertexArrays(1, &this->VAOId);
    glBindVertexArray(this->VAOId);

    glClearColor(.0f, .0f, .0f, 0.f);
    glPointSize(1);
    glViewport(0, 0, this->ScreenWidth, this->ScreenHeight);

    //generate coords and render them
    vtkm::Id3 dimensions(x, y, 1);
    vtkm::Vec<float, 3> origin(-4.f, -4.f, 0.0f);
    vtkm::Vec<float, 3> spacing(0.0075f, 0.0075f, 0.0f);

    vtkm::cont::ArrayHandleUniformPointCoordinates coords(dimensions, origin, spacing);
    vtkm::interop::TransferToOpenGL(coords, this->VBOState);
  }

  void render(vtkm::cont::DataSet& data)
  {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    vtkm::Int32 arraySize = (vtkm::Int32)data.GetNumberOfPoints();

    UploadData task(&this->ColorState,
                    data.GetField("colors", vtkm::cont::Field::Association::POINTS));
    vtkm::cont::TryExecute(task);

    vtkm::Float32 mvp[16] = { 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f,
                              0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 3.5f };

    GLint unifLoc = glGetUniformLocation(this->ShaderProgramId, "MVP");
    glUniformMatrix4fv(unifLoc, 1, GL_FALSE, mvp);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, *this->VBOState.GetHandle());
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glEnableClientState(GL_COLOR_ARRAY);
    glBindBuffer(GL_ARRAY_BUFFER, *this->ColorState.GetHandle());
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, 0);

    glDrawArrays(GL_POINTS, 0, arraySize);

    glDisableClientState(GL_COLOR_ARRAY);
    glDisableVertexAttribArray(0);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
  }
};

vtkm::cont::Timer gTimer;
vtkm::cont::DataSet* gData = nullptr;
GameOfLife* gFilter = nullptr;
RenderGameOfLife* gRenderer = nullptr;


vtkm::UInt32 stamp_acorn(std::vector<vtkm::UInt8>& input_state,
                         vtkm::UInt32 i,
                         vtkm::UInt32 j,
                         vtkm::UInt32 width,
                         vtkm::UInt32 height)
{
  (void)width;
  static vtkm::UInt8 acorn[5][9] = {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 1, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 1, 0, 0, 0, 0 },
    { 0, 1, 1, 0, 0, 1, 1, 1, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  };

  vtkm::UInt32 uindex = (i * height) + j;
  std::ptrdiff_t index = static_cast<std::ptrdiff_t>(uindex);
  for (vtkm::UInt32 x = 0; x < 5; ++x)
  {
    auto iter = input_state.begin() + index + static_cast<std::ptrdiff_t>((x * height));
    for (vtkm::UInt32 y = 0; y < 9; ++y, ++iter)
    {
      *iter = acorn[x][y];
    }
  }
  return j + 64;
}

void populate(std::vector<vtkm::UInt8>& input_state,
              vtkm::UInt32 width,
              vtkm::UInt32 height,
              vtkm::Float32 rate)
{
  std::random_device rd;
  std::mt19937 gen(rd());
  std::bernoulli_distribution d(rate);

  // Initially fill with random values
  {
    std::size_t index = 0;
    for (vtkm::UInt32 i = 0; i < width; ++i)
    {
      for (vtkm::UInt32 j = 0; j < height; ++j, ++index)
      {
        vtkm::UInt8 v = d(gen);
        input_state[index] = v;
      }
    }
  }

  //stamp out areas for acorns
  for (vtkm::UInt32 i = 2; i < (width - 64); i += 64)
  {
    for (vtkm::UInt32 j = 2; j < (height - 64);)
    {
      j = stamp_acorn(input_state, i, j, width, height);
    }
  }
}

int main(int argc, char** argv)
{
  auto opts =
    vtkm::cont::InitializeOptions::DefaultAnyDevice | vtkm::cont::InitializeOptions::Strict;
  vtkm::cont::Initialize(argc, argv, opts);

  glewExperimental = GL_TRUE;
  glutInit(&argc, argv);

  const vtkm::UInt32 width = 1024;
  const vtkm::UInt32 height = 768;

  const vtkm::UInt32 x = 1024;
  const vtkm::UInt32 y = 1024;

  vtkm::Float32 rate = 0.275f; //gives 1 27.5% of the time
  if (argc > 1)
  {
    rate = static_cast<vtkm::Float32>(std::atof(argv[1]));
    rate = std::max(0.0001f, rate);
    rate = std::min(0.9f, rate);
  }

  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
  glutInitWindowSize(width, height);
  glutCreateWindow("VTK-m Game Of Life");

  GLenum err = glewInit();
  if (GLEW_OK != err)
  {
    std::cout << "glewInit failed\n";
  }

  std::vector<vtkm::UInt8> input_state;
  input_state.resize(static_cast<std::size_t>(x * y), 0);
  populate(input_state, x, y, rate);


  vtkm::cont::DataSetBuilderUniform builder;
  vtkm::cont::DataSet data = builder.Create(vtkm::Id2(x, y));

  auto stateField =
    vtkm::cont::make_Field("state", vtkm::cont::Field::Association::POINTS, input_state);
  data.AddField(stateField);

  GameOfLife filter;
  RenderGameOfLife renderer(width, height, x, y);

  gData = &data;
  gFilter = &filter;
  gRenderer = &renderer;

  gTimer.Start();
  glutDisplayFunc([]() {
    const vtkm::Float32 c = static_cast<vtkm::Float32>(gTimer.GetElapsedTime());

    vtkm::cont::DataSet oData = gFilter->Execute(*gData, GameOfLifePolicy());
    gRenderer->render(oData);
    glutSwapBuffers();

    *gData = oData;

    if (c > 120)
    {
      //after 1 minute quit the demo
      exit(0);
    }
  });

  glutIdleFunc([]() { glutPostRedisplay(); });

  glutMainLoop();

  return 0;
}

#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
#pragma GCC diagnostic pop
#endif
