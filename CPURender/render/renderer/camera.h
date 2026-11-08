#pragma once
#include "../../common/frustum.h"
#include "../../common/transform.h"

enum CameraType
{
	PERSPECTIVE,
	ORTHOGRAPHIC,
};


class Camera
{
public:
	Camera(CameraType type);
	Camera(int _width, int _height, float _near, float _far);
	Camera(float _near, float _far, float _fovDegrees, int _width, int _height);
	
	void SetViewPort(int _width, int _height);
	void SetProj(float _near, float _far, float _fovDegrees);
	void SetProj(float _near, float _far);
	
	float GetFOVDegrees() const;
	float GetFOVRadians() const;
	float GetNear() const;
	float GetFar() const;
	
	glm::mat4x4 GetProjectionMatrix() const;
	glm::mat4x4 GetViewMatrix() const;
	
	Frustum GetFrustum() const;
	
	Transform transform;
private:
	void UpdateProjectionMatrix();
	Frustum ExtractFrustumPlanes() const;
	
	CameraType m_type;
	
	float m_widthViewport = 0.0f;
	float m_heightViewport = 0.0f;
	      
	float m_nearPlane = 0.001f;
	float m_farPlane = 100.0f;
	float m_fovDegrees = 60;
	
	glm::mat4x4 projectionMatrix;
};

