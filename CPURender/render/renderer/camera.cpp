#include "camera.h"

Camera::Camera(CameraType type) : m_type(type) {}

Camera::Camera(int _width, int _height, float _near, float _far) {
    m_widthViewport = static_cast<float>(_width);
    m_heightViewport = static_cast<float>(_height);
    m_nearPlane = _near;
    m_farPlane = _far;
    m_type = ORTHOGRAPHIC;
    UpdateProjectionMatrix();
}

Camera::Camera(float _near, float _far, float _fovDegrees, int _width, int _height) {
    m_nearPlane = _near;
    m_farPlane = _far;
    m_fovDegrees = _fovDegrees;
    m_widthViewport = static_cast<float>(_width);
    m_heightViewport = static_cast<float>(_height);
    m_type = PERSPECTIVE;
    UpdateProjectionMatrix();
}

void Camera::SetViewPort(int _width, int _height) {
    m_widthViewport = static_cast<float>(_width);
    m_heightViewport = static_cast<float>(_height);
    UpdateProjectionMatrix();
}

void Camera::SetProj(float _near, float _far, float _fovDegrees) {
    assert(m_type == PERSPECTIVE);
    m_nearPlane = _near;
    m_farPlane = _far;
    m_fovDegrees = _fovDegrees;
    UpdateProjectionMatrix();
}

void Camera::SetProj(float _near, float _far) {
    assert(m_type == ORTHOGRAPHIC);
    m_nearPlane = _near;
    m_farPlane = _far;
    UpdateProjectionMatrix();
}

float Camera::GetFOVDegrees() const {
    return m_fovDegrees;
}

float Camera::GetFOVRadians() const {
    return glm::radians(m_fovDegrees);
}

float Camera::GetNear() const {
    return m_nearPlane;
}

float Camera::GetFar() const {
    return m_farPlane;
}

glm::mat4x4 Camera::GetProjectionMatrix() const {
    return projectionMatrix;
}

glm::mat4x4 Camera::GetViewMatrix() const {
    return glm::lookAt(transform.getPosition(), transform.getPosition() + transform.getForward(), transform.getUp());
}

Frustum Camera::GetFrustum() const {
    return ExtractFrustumPlanes();
}

void Camera::UpdateProjectionMatrix() {
    switch (m_type)
    {
    case PERSPECTIVE:
        projectionMatrix = glm::perspectiveFov(glm::radians(m_fovDegrees), m_widthViewport, m_heightViewport, m_nearPlane, m_farPlane);
        break;
    case ORTHOGRAPHIC:
        float midWidth = m_widthViewport / 2.0f;
        float midHeight= m_heightViewport / 2.0f;
        projectionMatrix = glm::ortho(-midWidth, midWidth, -midHeight, midHeight, m_nearPlane, m_farPlane);
        break;
    }
}

Frustum Camera::ExtractFrustumPlanes() const
{
    glm::mat4 vp = projectionMatrix * GetViewMatrix();

    // Transpose so that t[i] gives row i of the original matrix
    glm::mat4 t = glm::transpose(vp);
    const glm::vec4 row0 = t[0];
    const glm::vec4 row1 = t[1];
    const glm::vec4 row2 = t[2];
    const glm::vec4 row3 = t[3];

    glm::vec4 left = row3 + row0;
    glm::vec4 right = row3 - row0;
    glm::vec4 bottom = row3 + row1;
    glm::vec4 top = row3 - row1;
    glm::vec4 nearP = row3 + row2;
    glm::vec4 farP = row3 - row2;

    Frustum frustum;
    frustum.leftFace.CreatePlaneFromVector(left);
    frustum.rightFace.CreatePlaneFromVector(right);
    frustum.bottomFace.CreatePlaneFromVector(bottom);
    frustum.topFace.CreatePlaneFromVector(top);
    frustum.nearFace.CreatePlaneFromVector(nearP);
    frustum.farFace.CreatePlaneFromVector(farP);
    return frustum;
}