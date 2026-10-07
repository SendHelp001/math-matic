#ifndef FAST_WIRE_SYSTEM_H
#define FAST_WIRE_SYSTEM_H

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/array.hpp>
#include <unordered_map>
#include <vector>

namespace godot {

struct Vector2iHash {
	size_t operator()(const Vector2i& v) const {
		return std::hash<int>()(v.x) ^ (std::hash<int>()(v.y) << 1);
	}
};

struct WireItem {
	float progress = 0.0f;
	Vector2i move_dir;
	Variant value;
	Node2D* visual = nullptr;
};

enum class WireType {
	NONE,
	CLASSIC,
	JUNCTION
};

struct WireCell {
	WireType type = WireType::NONE;
	Vector2i tile;
	
	// For CLASSIC
	Vector2i dir;
	bool has_item = false;
	WireItem item;
	
	// For JUNCTION
	std::vector<Vector2i> dirs;
	std::vector<WireItem> items;
	int rr_idx = 0;

	// Cache
	Vector2i cache_in_dir;
	PackedVector2Array cache_pts;
	Vector2 cache_arrow_pos;
	float cache_arrow_angle = 0.0f;
};

class FastWireSystem : public Node2D {
	GDCLASS(FastWireSystem, Node2D)

private:
	std::unordered_map<Vector2i, WireCell, Vector2iHash> wire_grid;
	std::unordered_map<Vector2i, bool, Vector2iHash> active_wires;
	Array preview_points;
	int current_wire_dir_index = 2;
	bool debug_mode = false;
	
	Color wire_color = Color(0.3, 0.8, 1.0);
	Color outline_color = Color(0.1, 0.1, 0.15);
	float wire_width = 4.0;
	float outline_width = 8.0;
	
	Ref<Texture2D> arrow_tex;
	Vector2 arrow_render_size;
	
	std::vector<Node2D*> item_visual_pool;
	
	Node* grid_system = nullptr;

	PackedVector2Array WIRE_DIRS;

	void trigger_cache_update(WireCell& cell);
	PackedVector2Array get_bezier_points(Vector2 p0, Vector2 p1, Vector2 p2, int segments);

	bool push_items_for_cell(WireCell& cell, bool moved_any);
	void interpolate_items_for_cell(WireCell& cell);
	bool accept_item(Vector2i tile, const Dictionary& itm_dict, Vector2i from_dir);

public:
	FastWireSystem();
	~FastWireSystem();

protected:
	static void _bind_methods();

public:
	void _ready() override;
	void _process(double delta) override;
	void _draw() override;

	void set_grid_system(Node* p_grid_system);
	Node* get_grid_system();

	bool get_debug_mode() const;
	void set_debug_mode(bool p_debug);

	Array get_preview_points() const;
	void set_preview_points(const Array& p_points);

	int get_current_wire_dir_index() const;
	void set_current_wire_dir_index(int p_idx);

	Vector2i pos_to_wire_tile(Vector2 pos);
	Vector2 wire_tile_to_pos(Vector2i tile);
	Vector2 snap_to_grid(Vector2 pos);
	
	Object* get_building_at(Vector2i next_t);
	
	Node2D* get_item_visual(Variant val);
	void recycle_item_visual(Node2D* vis);
	
	void notify_grid_changed(Vector2i t);
	
	void process_items(float delta, float game_speed);
	
	// API for GDScript
	bool has_wire(Vector2i pos);
	String get_wire_type(Vector2i pos);
	Vector2i get_wire_dir(Vector2i pos);
	Array get_junction_dirs(Vector2i pos);
	void set_wire(Vector2i pos, Vector2i dir);
	void set_junction(Vector2i pos, Array dirs);
	void remove_wire(Vector2i pos);
	void add_junction_dir(Vector2i pos, Vector2i dir);
	Dictionary get_wire_item(Vector2i pos); 
    Array get_wire_dirs_const();
};

} // namespace godot

#endif
