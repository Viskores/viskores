#ifndef vtk_m_rendering_View_h
#define vtk_m_rendering_View_h
#include <vtkm/Math.h>
#include <vtkm/Matrix.h>
#include <vtkm/VectorAnalysis.h>

namespace vtkm {
namespace rendering {

class View3D
{
public:
  vtkm::Vec<vtkm::Float32,3> Up;
  vtkm::Vec<vtkm::Float32,3> LookAt;
  vtkm::Vec<vtkm::Float32,3> Position;
  vtkm::Float32 NearPlane;
  vtkm::Float32 FarPlane;
  vtkm::Float32 FieldOfView;
  vtkm::Float32 AspectRatio;
  vtkm::Int32 Height;
  vtkm::Int32 Width;

  VTKM_CONT_EXPORT
  View3D()
  {
    Up[0] = 0.f;
    Up[1] = 1.f;
    Up[2] = 0.f;

    Position[0] = 0.f;
    Position[1] = 0.f;
    Position[2] = 0.f;

    LookAt[0] = 0.f;
    LookAt[1] = 0.f;
    LookAt[2] = 1.f;

    FieldOfView = 60.f;
    NearPlane = 1.0f;
    FarPlane = 100.0f;
    Height = 500;
    Width = 500;
  }

  VTKM_CONT_EXPORT
  vtkm::Matrix<vtkm::Float32,4,4> CreateViewMatrix() 
  {
    vtkm::Normalize(Up);
    vtkm::Matrix<vtkm::Float32,4,4> viewMatrix;
    vtkm::MatrixIdentity(viewMatrix);
    vtkm::Vec<vtkm::Float32,3> viewDir = Position - LookAt; //looking down the neg z axis
    vtkm::Normalize(viewDir);
    vtkm::Vec<vtkm::Float32,3> right = vtkm::Cross(Up,viewDir);
    vtkm::Vec<vtkm::Float32,3> ru = vtkm::Cross(right,viewDir);
    vtkm::Normalize(ru);
    vtkm::Normalize(right);
    viewMatrix(0,0) = right[0];
    viewMatrix(0,1) = right[1];
    viewMatrix(0,2) = right[2];
    viewMatrix(1,0) = ru[0];
    viewMatrix(1,1) = ru[1];
    viewMatrix(1,2) = ru[2];
    viewMatrix(2,0) = viewDir[0];
    viewMatrix(2,1) = viewDir[1];
    viewMatrix(2,2) = viewDir[2];

    viewMatrix(0,3) = -vtkm::dot(right,Position);
    viewMatrix(1,3) = -vtkm::dot(ru,Position);
    viewMatrix(2,3) = -vtkm::dot(viewDir,Position);
    return viewMatrix;
  }

  VTKM_CONT_EXPORT
  vtkm::Matrix<vtkm::Float32,4,4> CreateProjectionMatrix()
  {
    AspectRatio = vtkm::Float32(Width) / vtkm::Float32(Height);
    vtkm::Matrix<vtkm::Float32,4,4> projectionMatrix;
    vtkm::MatrixIdentity(projectionMatrix);
    vtkm::Float32 fovRad = (FieldOfView * 3.1415926f)/180.f;
    fovRad = vtkm::Tan( fovRad * 0.5f);
    std::cout<<"Field of View "<<fovRad<<std::endl;
    vtkm::Float32 size = NearPlane * fovRad;
    vtkm::Float32 left = -size * AspectRatio;
    vtkm::Float32 right = size * AspectRatio;
    vtkm::Float32 bottom = -size; 
    vtkm::Float32 top = size;

    projectionMatrix(0,0) = 2.f * NearPlane / (right - left);
    projectionMatrix(1,1) = 2.f * NearPlane / (top - bottom);
    projectionMatrix(0,2) = (right + left) / (right - left); 
    projectionMatrix(1,2) = (top + bottom) / (top - bottom); 
    projectionMatrix(2,2) = -(FarPlane + NearPlane)  / (FarPlane - NearPlane);
    projectionMatrix(3,2) = -1.f;
    projectionMatrix(2,3) = -(2.f * FarPlane * NearPlane) / (FarPlane - NearPlane);
    return projectionMatrix;
  }

  void operator=(const View3D &other)
  {
    this->Up = other.Up;
    this->LookAt = other.LookAt;
    this->Position = other.Position;
    this->NearPlane = other.NearPlane;
    this->FarPlane = other.FarPlane;
    this->FieldOfView = other.FieldOfView;
    this->Height = other.Height;
    this->Width = other.Width;
  }

  VTKM_CONT_EXPORT
  void GetRealViewport(vtkm::Float32 &l, vtkm::Float32 &r,
                       vtkm::Float32 &b, vtkm::Float32 &t)
  {
      l = -1;
      b = -1;
      r = 1;
      t = 1;
  }

  VTKM_CONT_EXPORT
  static void PrintMatrix(const vtkm::Matrix<vtkm::Float32,4,4> &mat)
  {
    std::cout<<mat(0,0)<<","<<mat(0,1)<<","<<mat(0,2)<<","<<mat(0,3)<<std::endl;
    std::cout<<mat(1,0)<<","<<mat(1,1)<<","<<mat(1,2)<<","<<mat(1,3)<<std::endl;
    std::cout<<mat(2,0)<<","<<mat(2,1)<<","<<mat(2,2)<<","<<mat(2,3)<<std::endl;
    std::cout<<mat(3,0)<<","<<mat(3,1)<<","<<mat(3,2)<<","<<mat(3,3)<<std::endl;
  }
};

class View2D
{
public:
    View2D() {}
};

}} // namespace vtkm::rendering
#endif // vtk_m_rendering_View_h
