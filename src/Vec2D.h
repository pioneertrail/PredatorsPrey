#ifndef VEC2D_H
#define VEC2D_H

#include <functional> // For std::hash

struct Vec2D {
    int x = 0;
    int y = 0;

    bool operator==(const Vec2D& other) const {
        return x == other.x && y == other.y;
    }
    bool operator!=(const Vec2D& other) const {
        return !(*this == other);
    }
};

// Custom hash function for Vec2D to use it in unordered_set/unordered_map
namespace std {
    template <> struct hash<Vec2D> {
        std::size_t operator()(const Vec2D& v) const noexcept {
            // Simple hash combination
            std::size_t h1 = std::hash<int>{}(v.x);
            std::size_t h2 = std::hash<int>{}(v.y);
            return h1 ^ (h2 << 1); // Combine hashes
        }
    };
}

#endif // VEC2D_H 