#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include "parser.cpp"

struct Plane {
    glm::vec3 normal;
    float d;

    void normalize() {
        float mag = glm::length(normal);
        normal = normal / mag;
        d = d / mag;
    }
};




class Camera {
public:
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
    float aspect = 1.7;
    float fov = 1;
    float near = 0.1f;
    float far = 1000.0f;
    int item_id = 0;
    bool moveable = true;
    float Yaw;
    float Pitch;
    float MovementSpeed;
    float MouseSensitivity;
    bool isCulling = true;

    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 2.0f),
        glm::vec3 up = glm::vec3(0.0f, 0.0f, 1.0f),
        float yaw = 90.0f, float pitch = 0.0f)
        : Front(glm::vec3(-2.0f, -2.0f, -2.0f)),
        MovementSpeed(0.5f),
        MouseSensitivity(0.1f) {
        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }

    glm::mat4 GetViewMatrix() {
        return glm::lookAt(Position, Position + Front, Up);
    }

    void ProcessKeyboard(char direction, float deltaTime) {
        float velocity = MovementSpeed * deltaTime;
        if (moveable) {
            if (direction == 'W')
                Position += Front * velocity;
            if (direction == 'S')
                Position -= Front * velocity;
            if (direction == 'A')
                Position -= Right * velocity;
            if (direction == 'D')
                Position += Right * velocity;
        }
    }

    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true) {
        if (moveable) {
            xoffset *= MouseSensitivity;
            yoffset *= MouseSensitivity;

            Yaw += xoffset;
            Pitch += yoffset;

            if (constrainPitch) {
                if (Pitch > 89.0f)
                    Pitch = 89.0f;
                if (Pitch < -89.0f)
                    Pitch = -89.0f;
            }

            updateCameraVectors();
        }
    }

    std::array<Plane, 6> extractFrustumPlanes(const glm::mat4& VP) {
        std::array<Plane, 6> planes;
        // Extract planes
        planes[0] = { glm::vec3(VP[0][3] + VP[0][0], VP[1][3] + VP[1][0], VP[2][3] + VP[2][0]), VP[3][3] + VP[3][0] }; // Left
        planes[1] = { glm::vec3(VP[0][3] - VP[0][0], VP[1][3] - VP[1][0], VP[2][3] - VP[2][0]), VP[3][3] - VP[3][0] }; // Right
        planes[2] = { glm::vec3(VP[0][3] + VP[0][1], VP[1][3] + VP[1][1], VP[2][3] + VP[2][1]), VP[3][3] + VP[3][1] }; // Bottom
        planes[3] = { glm::vec3(VP[0][3] - VP[0][1], VP[1][3] - VP[1][1], VP[2][3] - VP[2][1]), VP[3][3] - VP[3][1] }; // Top
        planes[4] = { glm::vec3(VP[0][3] + VP[0][2], VP[1][3] + VP[1][2], VP[2][3] + VP[2][2]), VP[3][3] + VP[3][2] }; // Near
        planes[5] = { glm::vec3(VP[0][3] - VP[0][2], VP[1][3] - VP[1][2], VP[2][3] - VP[2][2]), VP[3][3] - VP[3][2] }; // Far

        // Normalize planes
        for (auto& plane : planes) {
            plane.normalize();
        }

        return planes;
    }

    bool isSphereInsideFrustum(const SimpleJSONParser::BoundingSphere& sphere, const std::array<Plane, 6>& planes) {
        for (const auto& plane : planes) {
            float distance = glm::dot(plane.normal, sphere.center) + plane.d;
            if (distance < -sphere.radius) {
                return false; // Sphere is outside of this plane
            }
        }
        return true;
    }


private:
    void updateCameraVectors() {
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.z = sin(glm::radians(Pitch));
        front.y = -sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up = glm::normalize(glm::cross(Right, Front));
    }
};