// include/utils/force_directed_layout.hh
#ifndef FORCE_DIRECTED_LAYOUT_HH
#define FORCE_DIRECTED_LAYOUT_HH

#include <vector>
#include <string>
#include <unordered_map>
#include <cmath>
#include <algorithm>

struct Point {
    double x, y;
    double vx = 0, vy = 0;
    Point(double x = 0, double y = 0) : x(x), y(y) {}
};

class ForceDirectedLayout {
private:
    std::vector<std::string> nodes;
    std::unordered_map<std::string, Point> positions;
    std::unordered_map<std::string, std::string> node_to_group;
    std::unordered_set<std::string> fixed_nodes;

    // 物理参数
    double k = 100.0;
    double temperature = 100.0;
    int max_iter = 150;

    double attraction(double d) { return d * d / k; }
    double repulsion(double d) { return k * k / (d + 1e-6); }

public:
    void addNode(const std::string& name, double x = -1, double y = -1) {
        nodes.push_back(name);
        if (x >= 0 && y >= 0) {
            positions[name] = Point(x, y);
            fixed_nodes.insert(name);
        } else {
            positions[name] = Point(0, 0);  // 初始为 (0,0)，由 placement 决定
        }
    }

    void setGroup(const std::string& node, const std::string& group) {
        node_to_group[node] = group;
    }

    // 新增：为特定 group 设置布局方式
    void setGroupPlacement(
        const std::string& group_name,
        const std::vector<std::string>& members,
        const std::string& style,
        double center_x, double center_y, double radius = 80)
    {
        int n = members.size();
        if (n == 0) return;

        if (style == "grid") {
            int cols = std::max(1, (int)std::sqrt(n));
            int rows = (n + cols - 1) / cols;
            double w = 2 * radius, h = 2 * radius;
            double cell_w = w / std::max(1, cols - 1);
            double cell_h = h / std::max(1, rows - 1);

            for (int i = 0; i < n; ++i) {
                if (fixed_nodes.count(members[i])) continue;
                int row = i / cols;
                int col = i % cols;
                positions[members[i]] = Point(
                    center_x - w/2 + col * cell_w,
                    center_y - h/2 + row * cell_h
                );
            }
        }
        else if (style == "circle" || style == "radial") {
            for (int i = 0; i < n; ++i) {
                if (fixed_nodes.count(members[i])) continue;
                double angle = 2 * M_PI * i / n;
                positions[members[i]] = Point(
                    center_x + radius * cos(angle),
                    center_y + radius * sin(angle)
                );
            }
        }
        else if (style == "linear") {
            double spacing = 60;
            for (int i = 0; i < n; ++i) {
                if (fixed_nodes.count(members[i])) continue;
                positions[members[i]] = Point(
                    center_x - (n-1)*spacing/2 + i*spacing,
                    center_y
                );
            }
        }
        // 其他 style 可扩展
    }

    void setInitialPlacement(const std::string& type, double width = 600, double height = 400) {
        int n = nodes.size();
        if (n == 0) return;

        if (type == "grid") {
            int cols = std::max(1, (int)std::sqrt(n));
            int rows = (n + cols - 1) / cols;

            double cell_w = (width - 200) / std::max(1, cols - 1);
            double cell_h = (height - 200) / std::max(1, rows - 1);

            for (int i = 0; i < n; ++i) {
                if (fixed_nodes.count(nodes[i])) continue;
                int row = i / cols;
                int col = i % cols;
                positions[nodes[i]] = Point(
                    100 + col * cell_w,
                    100 + row * cell_h
                );
            }
        }
        else if (type == "circle") {
            double radius = std::min(width, height) * 0.35;
            double center_x = width / 2;
            double center_y = height / 2;

            for (int i = 0; i < n; ++i) {
                if (fixed_nodes.count(nodes[i])) continue;
                double angle = 2 * M_PI * i / n;
                positions[nodes[i]] = Point(
                    center_x + radius * cos(angle),
                    center_y + radius * sin(angle)
                );
            }
        }
        else {
            // 默认随机
            for (int i = 0; i < n; ++i) {
                if (fixed_nodes.count(nodes[i])) continue;
                positions[nodes[i]] = Point(
                    rand() % (int)(width - 200) + 100,
                    rand() % (int)(height - 200) + 100
                );
            }
        }
    }

    void run(double width = 600, double height = 400) {
        for (int iter = 0; iter < max_iter; ++iter) {
            // 清除速度
            for (auto& [name, pos] : positions) {
                if (fixed_nodes.count(name)) continue;
                pos.vx *= 0.8;
                pos.vy *= 0.8;
            }

            applyRepulsion();
            applyAttraction();
            applyGroupConfinement(width, height);

            for (auto& [name, pos] : positions) {
                if (fixed_nodes.count(name)) continue;

                pos.x += pos.vx * temperature / max_iter;
                pos.y += pos.vy * temperature / max_iter;

                // 边界限制
                pos.x = std::max(50.0, std::min(pos.x, width - 50));
                pos.y = std::max(50.0, std::min(pos.y, height - 50));

                pos.vx = 0;
                pos.vy = 0;
            }

            temperature *= 0.95;
        }
    }

    const std::unordered_map<std::string, Point>& getPositions() const {
        return positions;
    }

    // 查询是否手动固定
    bool isFixed(const std::string& node) const {
        return fixed_nodes.count(node) > 0;
    }

private:
    void applyRepulsion() {
        for (size_t i = 0; i < nodes.size(); ++i) {
            for (size_t j = i+1; j < nodes.size(); ++j) {
                const std::string& a = nodes[i];
                const std::string& b = nodes[j];

                double dx = positions[a].x - positions[b].x;
                double dy = positions[a].y - positions[b].y;
                double dist = std::sqrt(dx*dx + dy*dy) + 1e-6;

                double force = repulsion(dist);
                double fx = force * dx / dist;
                double fy = force * dy / dist;

                if (!fixed_nodes.count(a)) {
                    positions[a].vx -= fx;
                    positions[a].vy -= fy;
                }
                if (!fixed_nodes.count(b)) {
                    positions[b].vx += fx;
                    positions[b].vy += fy;
                }
            }
        }
    }

    void applyAttraction() {
        std::unordered_map<std::string, bool> has_edge;
        for (const auto& [node, group] : node_to_group) {
            has_edge[group + "_center"] = true;
        }
        // 可选：添加真实连接的引力
    }

    void applyGroupConfinement(double width, double height) {
        std::unordered_map<std::string, std::vector<Point>> group_centers;
        for (const auto& [node, group] : node_to_group) {
            if (!group.empty()) {
                group_centers[group].push_back(positions[node]);
            }
        }

        for (auto& [group, points] : group_centers) {
            if (points.empty()) continue;
            Point center(0, 0);
            for (auto& p : points) {
                center.x += p.x;
                center.y += p.y;
            }
            center.x /= points.size();
            center.y /= points.size();

            for (const std::string& node : nodes) {
                if (node_to_group[node] == group && !fixed_nodes.count(node)) {
                    double dx = positions[node].x - center.x;
                    double dy = positions[node].y - center.y;
                    double dist = std::sqrt(dx*dx + dy*dy) + 1e-6;
                    double force = 0.05 * dist;

                    positions[node].vx -= force * dx / dist;
                    positions[node].vy -= force * dy / dist;
                }
            }
        }
    }
};

#endif // FORCE_DIRECTED_LAYOUT_HH
