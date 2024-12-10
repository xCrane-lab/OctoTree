#include <iostream>
#include <vector>
#include <cmath>
#include <SFML/Window.hpp>
#include <SFML/OpenGL.hpp>

#define M_PI 3.14159265358979323846

// Структура для представления точки в 3D пространстве
struct Point3D {
    float x, y, z;
    bool isInsideSphere = false; // Флаг для обозначения, находится ли точка внутри сферы

    Point3D(float x, float y, float z) : x(x), y(y), z(z) {}
};

// Структура для узлов Octo-tree
struct OctreeNode {
    float x, y, z;          // Центр узла
    float size;             // Размер куба (длина ребра)
    std::vector<Point3D> points; // Точки, находящиеся внутри данного узла
    OctreeNode* children[8]; // Дочерние узлы

    OctreeNode(float x, float y, float z, float size) : x(x), y(y), z(z), size(size) {
        for (int i = 0; i < 8; ++i) {
            children[i] = nullptr;
        }
    }

    // Проверяет, содержится ли точка внутри текущего узла
    bool containsPoint(const Point3D& point) {
        return (point.x >= x - size / 2 && point.x <= x + size / 2 &&
            point.y >= y - size / 2 && point.y <= y + size / 2 &&
            point.z >= z - size / 2 && point.z <= z + size / 2);
    }

    // Проверяет, пересекается ли узел со сферой
    bool intersectsSphere(float sx, float sy, float sz, float sr) {
        float dx = std::max(x - size / 2, std::min(sx, x + size / 2)) - sx;
        float dy = std::max(y - size / 2, std::min(sy, y + size / 2)) - sy;
        float dz = std::max(z - size / 2, std::min(sz, z + size / 2)) - sz;
        return (dx * dx + dy * dy + dz * dz) <= (sr * sr);
    }
};

// Функция для вставки точки в Octo-tree
void insertPoint(OctreeNode* node, const Point3D& point, int maxPoints = 4) {
    if (!node->containsPoint(point)) return;

    // Если узел ещё не разделён и в нём меньше точек, чем maxPoints, добавляем точку
    if (node->points.size() < maxPoints && node->children[0] == nullptr) {
        node->points.push_back(point);
        return;
    }

    // Если узел переполнен, разделяем его на 8 дочерних узлов
    if (node->children[0] == nullptr) {
        float half = node->size / 2;
        float quarter = half / 2;
        for (int i = 0; i < 8; ++i) {
            float offsetX = (i & 1) ? quarter : -quarter;
            float offsetY = (i & 2) ? quarter : -quarter;
            float offsetZ = (i & 4) ? quarter : -quarter;
            node->children[i] = new OctreeNode(node->x + offsetX, node->y + offsetY, node->z + offsetZ, half);
        }

        // Перемещаем существующие точки в дочерние узлы
        for (const auto& p : node->points) {
            for (int i = 0; i < 8; ++i) {
                insertPoint(node->children[i], p);
            }
        }
        node->points.clear();
    }

    // Вставляем новую точку в соответствующий дочерний узел
    for (int i = 0; i < 8; ++i) {
        insertPoint(node->children[i], point);
    }
}

// Функция для поиска точек внутри сферы
void findPointsInSphere(OctreeNode* node, float sx, float sy, float sz, float sr, std::vector<Point3D*>& result) {
    if (!node || !node->intersectsSphere(sx, sy, sz, sr)) return;

    // Проверяем точки, находящиеся в текущем узле
    for (auto& point : node->points) {
        float dx = point.x - sx;
        float dy = point.y - sy;
        float dz = point.z - sz;
        if (dx * dx + dy * dy + dz * dz <= sr * sr) {
            point.isInsideSphere = true;
            result.push_back(&point);
        }
        else {
            point.isInsideSphere = false;
        }
    }

    // Рекурсивно проверяем дочерние узлы
    for (int i = 0; i < 8; ++i) {
        findPointsInSphere(node->children[i], sx, sy, sz, sr, result);
    }
}

// Функция для рисования куба в OpenGL
void drawCube(float x, float y, float z, float size) {
    float half = size / 2;

    glBegin(GL_LINES);
    glColor3f(0.0f, 0.0f, 1.0f); // Синий цвет для рёбер куба

    // Передняя грань
    glVertex3f(x - half, y - half, z - half);
    glVertex3f(x + half, y - half, z - half);

    glVertex3f(x + half, y - half, z - half);
    glVertex3f(x + half, y + half, z - half);

    glVertex3f(x + half, y + half, z - half);
    glVertex3f(x - half, y + half, z - half);

    glVertex3f(x - half, y + half, z - half);
    glVertex3f(x - half, y - half, z - half);

    // Задняя грань
    glVertex3f(x - half, y - half, z + half);
    glVertex3f(x + half, y - half, z + half);

    glVertex3f(x + half, y - half, z + half);
    glVertex3f(x + half, y + half, z + half);

    glVertex3f(x + half, y + half, z + half);
    glVertex3f(x - half, y + half, z + half);

    glVertex3f(x - half, y + half, z + half);
    glVertex3f(x - half, y - half, z + half);

    // Соединяющие рёбра
    glVertex3f(x - half, y - half, z - half);
    glVertex3f(x - half, y - half, z + half);

    glVertex3f(x + half, y - half, z - half);
    glVertex3f(x + half, y - half, z + half);

    glVertex3f(x + half, y + half, z - half);
    glVertex3f(x + half, y + half, z + half);

    glVertex3f(x - half, y + half, z - half);
    glVertex3f(x - half, y + half, z + half);

    glEnd();
}

// Функция для рисования сферы в OpenGL
void drawSphere(float x, float y, float z, float radius, int slices, int stacks) {
    for (int i = 0; i <= stacks; ++i) {
        float lat0 = M_PI * (-0.5 + (float)(i - 1) / stacks);
        float z0 = sin(lat0) * radius;
        float zr0 = cos(lat0) * radius;

        float lat1 = M_PI * (-0.5 + (float)i / stacks);
        float z1 = sin(lat1) * radius;
        float zr1 = cos(lat1) * radius;

        glBegin(GL_LINE_LOOP); // Рисуем сферу как проволочную модель
        for (int j = 0; j <= slices; ++j) {
            float lng = 2 * M_PI * (float)j / slices;
            float x1 = cos(lng);
            float y1 = sin(lng);

            glVertex3f(x + x1 * zr0, y + y1 * zr0, z + z0);
            glVertex3f(x + x1 * zr1, y + y1 * zr1, z + z1);
        }
        glEnd();
    }
}

// Функция для рисования Octo-tree и точек
void drawOctree(OctreeNode* node) {
    if (!node) return;

    drawCube(node->x, node->y, node->z, node->size);

    glPointSize(5.0f); // Устанавливаем размер точек
    for (const auto& point : node->points) {
        glColor3f(point.isInsideSphere ? 1.0f : 0.0f, point.isInsideSphere ? 0.0f : 1.0f, 0.0f); // Красный для точек внутри сферы, синий для остальных
        glBegin(GL_POINTS);
        glVertex3f(point.x, point.y, point.z);
        glEnd();
    }

    for (int i = 0; i < 8; ++i) {
        drawOctree(node->children[i]);
    }
}

int main() {
    // Генерация случайных точек
    std::vector<Point3D> points;
    for (int i = 0; i < 100; ++i) {
        points.emplace_back(rand() % 200 - 100, rand() % 200 - 100, rand() % 200 - 100);
    }

    // Построение Octo-tree
    OctreeNode* root = new OctreeNode(0, 0, 0, 200);
    for (const auto& point : points) {
        insertPoint(root, point);
    }

    // Параметры сферы
    float sphereX = 0, sphereY = 0, sphereZ = 0, sphereRadius = 50;

    // Инициализация окна SFML с OpenGL контекстом
    sf::Window window(sf::VideoMode(800, 600), "3D Octo-tree Visualization", sf::Style::Default, sf::ContextSettings(24));
    window.setActive(true);

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    float angleX = 0.0f, angleY = 0.0f;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Left) angleY -= 5.0f;
                if (event.key.code == sf::Keyboard::Right) angleY += 5.0f;
                if (event.key.code == sf::Keyboard::Up) angleX -= 5.0f;
                if (event.key.code == sf::Keyboard::Down) angleX += 5.0f;
                if (event.key.code == sf::Keyboard::W) sphereY += 5.0f;
                if (event.key.code == sf::Keyboard::S) sphereY -= 5.0f;
                if (event.key.code == sf::Keyboard::A) sphereX -= 5.0f;
                if (event.key.code == sf::Keyboard::D) sphereX += 5.0f;
                if (event.key.code == sf::Keyboard::Q) sphereRadius += 5.0f;
                if (event.key.code == sf::Keyboard::E) sphereRadius -= 5.0f;
            }
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glFrustum(-1, 1, -1, 1, 1, 1000);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glTranslatef(0.0f, 0.0f, -200.0f); // Перемещаем сцену ближе
        glRotatef(angleX, 1.0f, 0.0f, 0.0f);
        glRotatef(angleY, 0.0f, 1.0f, 0.0f);

        // Поиск точек внутри сферы
        std::vector<Point3D*> pointsInSphere;
        findPointsInSphere(root, sphereX, sphereY, sphereZ, sphereRadius, pointsInSphere);

        // Рисуем Octo-tree
        drawOctree(root);

        // Рисуем сферу
        glColor3f(0.0f, 1.0f, 0.0f); // Зелёный цвет для сферы
        drawSphere(sphereX, sphereY, sphereZ, sphereRadius, 16, 16);

        window.display();
    }

    return 0;
}