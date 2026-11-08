#pragma once
#include <vector>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/ext/quaternion_float.hpp>

struct Transform {
    Transform();
    ~Transform();

    void setParent(Transform* _parent);

    void translate(const glm::vec3& _delta);
    void rotate(const glm::quat& _deltaQuat);
    void rotate(const glm::vec3& _deltaEuler);
    void scale(const glm::vec3& _factor);

    void setPosition(const glm::vec3& _pos);
    void setRotation(const glm::quat& _quat);
    void setRotation(const glm::vec3& _euler);
    void setScale(const glm::vec3& _scale);

    void localTranslate(const glm::vec3& _delta);
    void localRotate(const glm::quat& _deltaQuat);
    void localRotate(const glm::vec3& _deltaEuler);
    void localScale(const glm::vec3& _factor);

    void setLocalPosition(const glm::vec3& _pos);
    void setLocalRotation(const glm::quat& _quat);
    void setLocalRotation(const glm::vec3& _euler);
    void setLocalScale(const glm::vec3& _scale);

    void setWorldTransform(const glm::vec3& _pos, const glm::quat& _quat, const glm::vec3& _scale);

    glm::vec3 getLocalPosition() const { return m_localPosition; }
    glm::quat getLocalRotation() const { return m_localRotation; }
    glm::vec3 getLocalScale() const { return m_localScale; }

    glm::mat4x4 getWorldMatrix() const;
    glm::mat4x4 getTransposedWorldMatrix() const;
    glm::vec3 getPosition() const;
    glm::quat getRotation() const;
    glm::vec3 getScale() const;

    glm::vec3 getForward() const;
    glm::vec3 getRight() const;
    glm::vec3 getUp() const;

    void lookAt(const glm::vec3& _target, const glm::vec3& _up = { 0.f, 1.f, 0.f });
    void lookTo(const glm::vec3& _direction, const glm::vec3& _up = { 0.f, 1.f, 0.f });

private:
    void setDirty() const;
    void updateValues() const;

    Transform* m_parent = nullptr;
    std::vector<Transform*> m_childrens = {};

    glm::vec3 m_localPosition = { 0.f, 0.f, 0.f };
    glm::quat m_localRotation = { 1.f, 0.f, 0.f, 0.f };
    glm::vec3 m_localScale = { 1.f, 1.f, 1.f };

    mutable glm::mat4x4 m_worldMatrix;
    mutable glm::vec3 m_worldPosition = { 0.f, 0.f, 0.f };
    mutable glm::quat m_worldRotation = { 1.f, 0.f, 0.f, 0.f };
    mutable glm::vec3 m_worldScale = { 1.f, 1.f, 1.f };

    mutable bool m_isDirty = true;
};
