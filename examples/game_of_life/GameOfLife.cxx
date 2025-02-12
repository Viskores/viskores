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

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/CellSetStructured.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/Initialize.h>
#include <viskores/cont/Timer.h>

#include <viskores/interop/TransferToOpenGL.h>

#include <viskores/filter/Filter.h>
#include <viskores/worklet/WorkletPointNeighborhood.h>

#include <viskores/cont/Invoker.h>
#include <viskores/cont/TryExecute.h>

//Suppress warnings about glut being deprecated on OSX
#if (defined(VISKORES_GCC) || defined(VISKORES_CLANG))
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

struct UpdateLifeState : public viskores::worklet::WorkletPointNeighborhood
{
  using CountingHandle = viskores::cont::ArrayHandleCounting<viskores::Id>;

  using ControlSignature = void(CellSetIn,
                                FieldInNeighborhood prevstate,
                                FieldOut state,
                                FieldOut color);

  using ExecutionSignature = void(_2, _3, _4);

  template <typename NeighIn>
  VISKORES_EXEC void operator()(const NeighIn& prevstate,
                            viskores::UInt8& state,
                            viskores::Vec4ui_8& color) const
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
    color[1] = static_cast<viskores::UInt8>(state * (100 + (count * 32)));
    color[2] = (state && !current) ? static_cast<viskores::UInt8>(100 + (count * 32)) : 0;
    color[3] = 255; //alpha channel
  }
};


class GameOfLife : public viskores::filter::Filter
{
public:
  VISKORES_CONT GameOfLife() { this->SetActiveField("state", viskores::cont::Field::Association::Points); }

  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override
  {
    viskores::cont::ArrayHandle<viskores::UInt8> state;
    viskores::cont::ArrayHandle<viskores::UInt8> prevstate;
    viskores::cont::ArrayHandle<viskores::Vec4ui_8> colors;

    //get the coordinate system we are using for the 2D area
    viskores::cont::CellSetStructured<2> cells;
    input.GetCellSet().AsCellSet(cells);

    //get the previous state of the game
    this->GetFieldFromDataSet(input).GetData().AsArrayHandle(prevstate);

    //Update the game state
    this->Invoke(UpdateLifeState{}, cells, prevstate, state, colors);

    //save the results
    viskores::cont::DataSet output =
      this->CreateResultFieldPoint(input, this->GetActiveFieldName(), state);
    output.AddField(viskores::cont::make_FieldPoint("colors", colors));
    return output;
  }
};

struct UploadData
{
  viskores::interop::BufferState* ColorState;
  viskores::cont::Field Colors;

  UploadData(viskores::interop::BufferState* cs, viskores::cont::Field colors)
    : ColorState(cs)
    , Colors(colors)
  {
  }
  template <typename DeviceAdapterTag>
  bool operator()(DeviceAdapterTag device)
  {
    viskores::cont::ArrayHandle<viskores::Vec4ui_8> colors;
    this->Colors.GetData().AsArrayHandle(colors);
    viskores::interop::TransferToOpenGL(colors, *this->ColorState, device);
    return true;
  }
};

struct RenderGameOfLife
{
  viskores::Int32 ScreenWidth;
  viskores::Int32 ScreenHeight;
  GLuint ShaderProgramId;
  GLuint VAOId;
  viskores::interop::BufferState VBOState;
  viskores::interop::BufferState ColorState;

  RenderGameOfLife(viskores::Int32 width, viskores::Int32 height, viskores::Int32 x, viskores::Int32 y)
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
    viskores::Id3 dimensions(x, y, 1);
    viskores::Vec<float, 3> origin(-4.f, -4.f, 0.0f);
    viskores::Vec<float, 3> spacing(0.0075f, 0.0075f, 0.0f);

    viskores::cont::ArrayHandleUniformPointCoordinates coords(dimensions, origin, spacing);
    viskores::interop::TransferToOpenGL(coords, this->VBOState);
  }

  void render(viskores::cont::DataSet& data)
  {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    viskores::Int32 arraySize = (viskores::Int32)data.GetNumberOfPoints();

    UploadData task(&this->ColorState,
                    data.GetField("colors", viskores::cont::Field::Association::Points));
    viskores::cont::TryExecute(task);

    viskores::Float32 mvp[16] = { 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f,
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

viskores::cont::Timer gTimer;
viskores::cont::DataSet* gData = nullptr;
GameOfLife* gFilter = nullptr;
RenderGameOfLife* gRenderer = nullptr;


viskores::UInt32 stamp_acorn(std::vector<viskores::UInt8>& input_state,
                         viskores::UInt32 i,
                         viskores::UInt32 j,
                         viskores::UInt32 width,
                         viskores::UInt32 height)
{
  (void)width;
  static viskores::UInt8 acorn[5][9] = {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 1, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 1, 0, 0, 0, 0 },
    { 0, 1, 1, 0, 0, 1, 1, 1, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  };

  viskores::UInt32 uindex = (i * height) + j;
  std::ptrdiff_t index = static_cast<std::ptrdiff_t>(uindex);
  for (viskores::UInt32 x = 0; x < 5; ++x)
  {
    auto iter = input_state.begin() + index + static_cast<std::ptrdiff_t>((x * height));
    for (viskores::UInt32 y = 0; y < 9; ++y, ++iter)
    {
      *iter = acorn[x][y];
    }
  }
  return j + 64;
}

void populate(std::vector<viskores::UInt8>& input_state,
              viskores::UInt32 width,
              viskores::UInt32 height,
              viskores::Float32 rate)
{
  std::random_device rd;
  std::mt19937 gen(rd());
  std::bernoulli_distribution d(rate);

  // Initially fill with random values
  {
    std::size_t index = 0;
    for (viskores::UInt32 i = 0; i < width; ++i)
    {
      for (viskores::UInt32 j = 0; j < height; ++j, ++index)
      {
        viskores::UInt8 v = d(gen);
        input_state[index] = v;
      }
    }
  }

  //stamp out areas for acorns
  for (viskores::UInt32 i = 2; i < (width - 64); i += 64)
  {
    for (viskores::UInt32 j = 2; j < (height - 64);)
    {
      j = stamp_acorn(input_state, i, j, width, height);
    }
  }
}

int main(int argc, char** argv)
{
  auto opts =
    viskores::cont::InitializeOptions::DefaultAnyDevice | viskores::cont::InitializeOptions::Strict;
  viskores::cont::Initialize(argc, argv, opts);

  glewExperimental = GL_TRUE;
  glutInit(&argc, argv);

  const viskores::UInt32 width = 1024;
  const viskores::UInt32 height = 768;

  const viskores::UInt32 x = 1024;
  const viskores::UInt32 y = 1024;

  viskores::Float32 rate = 0.275f; //gives 1 27.5% of the time
  if (argc > 1)
  {
    rate = static_cast<viskores::Float32>(std::atof(argv[1]));
    rate = std::max(0.0001f, rate);
    rate = std::min(0.9f, rate);
  }

  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
  glutInitWindowSize(width, height);
  glutCreateWindow("Viskores Game Of Life");

  GLenum err = glewInit();
  if (GLEW_OK != err)
  {
    std::cout << "glewInit failed\n";
  }

  std::vector<viskores::UInt8> input_state;
  input_state.resize(static_cast<std::size_t>(x * y), 0);
  populate(input_state, x, y, rate);


  viskores::cont::DataSetBuilderUniform builder;
  viskores::cont::DataSet data = builder.Create(viskores::Id2(x, y));

  auto stateField = viskores::cont::make_FieldMove(
    "state", viskores::cont::Field::Association::Points, std::move(input_state));
  data.AddField(stateField);

  GameOfLife filter;
  RenderGameOfLife renderer(width, height, x, y);

  gData = &data;
  gFilter = &filter;
  gRenderer = &renderer;

  gTimer.Start();
  glutDisplayFunc([]() {
    const viskores::Float32 c = static_cast<viskores::Float32>(gTimer.GetElapsedTime());

    viskores::cont::DataSet oData = gFilter->Execute(*gData);
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

#if (defined(VISKORES_GCC) || defined(VISKORES_CLANG))
#pragma GCC diagnostic pop
#endif
