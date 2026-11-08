#include "Transform.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace glm;

quat eulerToQuaternion(const vec3& _euler) {
    return quat{_euler};
}

Transform::Transform() {
    m_worldMatrix = identity<mat4x4>();
}

Transform::~Transform() {
    if (m_parent) {
        auto& c = m_parent->m_childrens;
        auto it = std::find(c.begin(), c.end(), this);
        if (it != c.end())
            c.erase(it);
    }
    
    for (auto* child : m_childrens) {
        child->m_parent = nullptr;
        child->setDirty();
    }
    m_childrens.clear();
}

void Transform::setDirty() const {
    if (m_isDirty)
        return;

    m_isDirty = true;

    for (auto* child : m_childrens)
        child->setDirty();
}

void Transform::setParent(Transform* _parent) {
    if (m_parent == _parent)
        return;

    if (m_parent) {
        auto& c = m_parent->m_childrens;
        auto it = std::find(c.begin(), c.end(), this);
        if (it != c.end())
            c.erase(it);
    }

    m_parent = _parent;

    if (m_parent)
        m_parent->m_childrens.push_back(this);

    setDirty();
}

void Transform::setPosition(const vec3& _pos) {
    if (m_parent) {
        mat4x4 invParentWorld = inverse(m_parent->getWorldMatrix());
        m_localPosition = vec3(invParentWorld * vec4(_pos, 1.0f));
    }
    else {
        m_localPosition = _pos;
    }
    setDirty();
}

void Transform::setRotation(const quat& _quat) {
    quat worldQuat = normalize(_quat);

    if (m_parent) {
        quat parentWorldRot = m_parent->getRotation();
        m_localRotation = normalize(inverse(parentWorldRot) * worldQuat);
    }
    else {
        m_localRotation = worldQuat;
    }
    setDirty();
}

void Transform::setRotation(const vec3& _euler) {
    setRotation(eulerToQuaternion(_euler));
}

void Transform::setScale(const vec3& _scale) {
    if (m_parent)
        m_localScale = _scale / m_parent->getScale();
    else
        m_localScale = _scale;
    setDirty();
}

void Transform::setWorldTransform(const vec3& _pos, const quat& _quat, const vec3& _scale) {
    quat normalizedQuat = normalize(_quat);

    if (m_parent) {
        mat4x4 invParentMat = inverse(m_parent->getWorldMatrix());
        quat   invParentRot = inverse(m_parent->getRotation());
        vec3   parentSca    = m_parent->getScale();

        m_localPosition = vec3(invParentMat * vec4(_pos, 1.0f));
        m_localRotation = normalize(invParentRot * normalizedQuat);
        m_localScale    = _scale / parentSca;
    }
    else {
        m_localPosition = _pos;
        m_localRotation = normalizedQuat;
        m_localScale    = _scale;
    }

    setDirty();
}

void Transform::localTranslate(const vec3& _delta) {
    m_localPosition += _delta;
    setDirty();
}

void Transform::localRotate(const quat& _deltaQuat) {
    m_localRotation = normalize(m_localRotation * normalize(_deltaQuat));
    setDirty();
}

void Transform::localRotate(const vec3& _deltaEuler) {
    localRotate(eulerToQuaternion(_deltaEuler));
}

void Transform::localScale(const vec3& _factor) {
    m_localScale *= _factor;
    setDirty();
}

void Transform::translate(const vec3& _delta) {
    updateValues();
    setPosition(m_worldPosition + _delta);
}

void Transform::rotate(const quat& _deltaQuat) {
    updateValues();
    setRotation(normalize(_deltaQuat) * m_worldRotation);
}

void Transform::rotate(const vec3& _deltaEuler) {
    rotate(eulerToQuaternion(_deltaEuler));
}

void Transform::scale(const vec3& _factor) {
    updateValues();
    setScale(m_worldScale * _factor);
}

void Transform::setLocalPosition(const vec3& _pos) {
    m_localPosition = _pos;
    setDirty();
}

void Transform::setLocalRotation(const quat& _quat) {
    m_localRotation = normalize(_quat);
    setDirty();
}

void Transform::setLocalRotation(const vec3& _euler) {
    setLocalRotation(eulerToQuaternion(_euler));
}

void Transform::setLocalScale(const vec3& _scale) {
    m_localScale = _scale;
    setDirty();
}

void Transform::lookAt(const vec3& _target, const vec3& _up) {
    lookTo(_target - getPosition(), _up);
}

void Transform::lookTo(const vec3& _direction, const vec3& _up) {
    if (dot(_direction, _direction) < 1e-12f)
        return;
    setRotation(quatLookAtRH(normalize(_direction), _up));
}

void Transform::updateValues() const {
    if (!m_isDirty)
        return;
    
    mat4x4 localMat = glm::translate(identity<mat4x4>(), m_localPosition)
                    * mat4_cast(m_localRotation)
                    * glm::scale(identity<mat4x4>(), m_localScale);

    if (m_parent) {
        mat4x4 worldMat = m_parent->getWorldMatrix() * localMat;
        m_worldMatrix   = worldMat;
        m_worldPosition = vec3(worldMat[3]);
        m_worldRotation = normalize(m_parent->getRotation() * m_localRotation);
        m_worldScale    = m_localScale * m_parent->getScale();
    }
    else {
        m_worldMatrix   = localMat;
        m_worldPosition = m_localPosition;
        m_worldRotation = m_localRotation;
        m_worldScale    = m_localScale;
    }

    m_isDirty = false;
}

mat4x4 Transform::getWorldMatrix() const {
    updateValues();
    return m_worldMatrix;
}

mat4x4 Transform::getTransposedWorldMatrix() const {
    updateValues();
    return transpose(m_worldMatrix);
}

vec3 Transform::getPosition() const {
    updateValues();
    return m_worldPosition;
}

quat Transform::getRotation() const {
    updateValues();
    return m_worldRotation;
}

vec3 Transform::getScale() const {
    updateValues();
    return m_worldScale;
}

vec3 Transform::getForward() const {
    return normalize(getRotation() * vec3(0.0f, 0.0f, -1.0f));
}

vec3 Transform::getRight() const {
    return normalize(getRotation() * vec3(1.0f, 0.0f, 0.0f));
}

vec3 Transform::getUp() const {
    return normalize(getRotation() * vec3(0.0f, 1.0f, 0.0f));
}