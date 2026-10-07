#include "fast_wire_system.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/sprite2d.hpp>
#include <godot_cpp/classes/camera2d.hpp>

using namespace godot;

FastWireSystem::FastWireSystem() {
	WIRE_DIRS.push_back(Vector2i(0, -1));
	WIRE_DIRS.push_back(Vector2i(1, -1));
	WIRE_DIRS.push_back(Vector2i(1, 0));
	WIRE_DIRS.push_back(Vector2i(1, 1));
	WIRE_DIRS.push_back(Vector2i(0, 1));
	WIRE_DIRS.push_back(Vector2i(-1, 1));
	WIRE_DIRS.push_back(Vector2i(-1, 0));
	WIRE_DIRS.push_back(Vector2i(-1, -1));
}

FastWireSystem::~FastWireSystem() {
}

void FastWireSystem::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_grid_system", "grid_system"), &FastWireSystem::set_grid_system);
	ClassDB::bind_method(D_METHOD("get_grid_system"), &FastWireSystem::get_grid_system);
	ClassDB::bind_method(D_METHOD("get_debug_mode"), &FastWireSystem::get_debug_mode);
	ClassDB::bind_method(D_METHOD("set_debug_mode", "debug"), &FastWireSystem::set_debug_mode);
	ClassDB::bind_method(D_METHOD("get_preview_points"), &FastWireSystem::get_preview_points);
	ClassDB::bind_method(D_METHOD("set_preview_points", "points"), &FastWireSystem::set_preview_points);
	ClassDB::bind_method(D_METHOD("get_current_wire_dir_index"), &FastWireSystem::get_current_wire_dir_index);
	ClassDB::bind_method(D_METHOD("set_current_wire_dir_index", "idx"), &FastWireSystem::set_current_wire_dir_index);
	
	ClassDB::bind_method(D_METHOD("pos_to_wire_tile", "pos"), &FastWireSystem::pos_to_wire_tile);
	ClassDB::bind_method(D_METHOD("wire_tile_to_pos", "tile"), &FastWireSystem::wire_tile_to_pos);
	ClassDB::bind_method(D_METHOD("snap_to_grid", "pos"), &FastWireSystem::snap_to_grid);
	
	ClassDB::bind_method(D_METHOD("process_items", "delta", "game_speed"), &FastWireSystem::process_items);
	
	ClassDB::bind_method(D_METHOD("has_wire", "pos"), &FastWireSystem::has_wire);
	ClassDB::bind_method(D_METHOD("get_wire_type", "pos"), &FastWireSystem::get_wire_type);
	ClassDB::bind_method(D_METHOD("get_wire_dir", "pos"), &FastWireSystem::get_wire_dir);
	ClassDB::bind_method(D_METHOD("get_junction_dirs", "pos"), &FastWireSystem::get_junction_dirs);
	ClassDB::bind_method(D_METHOD("set_wire", "pos", "dir"), &FastWireSystem::set_wire);
	ClassDB::bind_method(D_METHOD("set_junction", "pos", "dirs"), &FastWireSystem::set_junction);
	ClassDB::bind_method(D_METHOD("remove_wire", "pos"), &FastWireSystem::remove_wire);
	ClassDB::bind_method(D_METHOD("add_junction_dir", "pos", "dir"), &FastWireSystem::add_junction_dir);
	ClassDB::bind_method(D_METHOD("get_wire_dirs_const"), &FastWireSystem::get_wire_dirs_const);
	ClassDB::bind_method(D_METHOD("accept_item", "tile", "itm_dict", "from_dir"), &FastWireSystem::accept_item);
	ClassDB::bind_method(D_METHOD("get_item_visual", "val"), &FastWireSystem::get_item_visual);
	ClassDB::bind_method(D_METHOD("recycle_item_visual", "vis"), &FastWireSystem::recycle_item_visual);
	ClassDB::bind_method(D_METHOD("get_wire_item", "pos"), &FastWireSystem::get_wire_item);
	ClassDB::bind_method(D_METHOD("get_building_at", "pos"), &FastWireSystem::get_building_at);	
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "grid_system"), "set_grid_system", "get_grid_system");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "debug_mode"), "set_debug_mode", "get_debug_mode");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "preview_points"), "set_preview_points", "get_preview_points");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "current_wire_dir_index"), "set_current_wire_dir_index", "get_current_wire_dir_index");
}

void FastWireSystem::set_grid_system(Node* p_grid_system) { grid_system = p_grid_system; }
Node* FastWireSystem::get_grid_system() { return grid_system; }

bool FastWireSystem::get_debug_mode() const { return debug_mode; }
void FastWireSystem::set_debug_mode(bool p_debug) { debug_mode = p_debug; queue_redraw(); }

Array FastWireSystem::get_preview_points() const { return preview_points; }
void FastWireSystem::set_preview_points(const Array& p_points) { preview_points = p_points; queue_redraw(); }

int FastWireSystem::get_current_wire_dir_index() const { return current_wire_dir_index; }
void FastWireSystem::set_current_wire_dir_index(int p_idx) { current_wire_dir_index = p_idx; queue_redraw(); }

Array FastWireSystem::get_wire_dirs_const() {
	Array arr;
	for (int i = 0; i < WIRE_DIRS.size(); i++) arr.push_back(WIRE_DIRS[i]);
	return arr;
}

void FastWireSystem::_ready() {
	set_z_index(1);
	
	ResourceLoader* rl = ResourceLoader::get_singleton();
	arrow_tex = rl->load("res://asset/wireArrow.png");
	if (arrow_tex.is_valid()) {
		Vector2 ts = arrow_tex->get_size();
		float scale_factor = std::min(16.0f / ts.x, 16.0f / ts.y);
		arrow_render_size = ts * scale_factor;
	}
}

void FastWireSystem::_process(double delta) {
	queue_redraw();
}

Vector2i FastWireSystem::pos_to_wire_tile(Vector2 pos) {
	return Vector2i(Math::floor(pos.x / 16.0f), Math::floor(pos.y / 16.0f));
}

Vector2 FastWireSystem::wire_tile_to_pos(Vector2i tile) {
	return Vector2(tile.x * 16.0f + 8.0f, tile.y * 16.0f + 8.0f);
}

Vector2 FastWireSystem::snap_to_grid(Vector2 pos) {
	return Vector2(Math::floor(pos.x / 16.0f) * 16.0f + 8.0f, Math::floor(pos.y / 16.0f) * 16.0f + 8.0f);
}

Object* FastWireSystem::get_building_at(Vector2i next_t) {
	if (!grid_system) return nullptr;
	Vector2 world_pos = wire_tile_to_pos(next_t);
	
	Variant cell_var = grid_system->call("pos_to_cell", world_pos);
	if (cell_var.get_type() != Variant::VECTOR2I) return nullptr;
	Vector2i cell_4x4 = cell_var;
	
	Dictionary grid_data = grid_system->get("grid_data");
	if (grid_data.has(cell_4x4)) {
		Dictionary b = grid_data[cell_4x4];
		String type = b.get("type", "");
		if (type == "core" || type == "miner" || type == "adder" || type == "turret") {
			return b.get("ref", Variant()).operator Object *();
		}
	}
	return nullptr;
}

Node2D* FastWireSystem::get_item_visual(Variant val) {
	Node2D* container = nullptr;
	Label* lbl = nullptr;
	
	if (!item_visual_pool.empty()) {
		container = item_visual_pool.back();
		item_visual_pool.pop_back();
		container->show();
		lbl = Object::cast_to<Label>(container->get_child(1));
	} else {
		container = memnew(Node2D);
		container->set_z_index(20);
		
		Sprite2D* bg = memnew(Sprite2D);
		ResourceLoader* rl = ResourceLoader::get_singleton();
		bg->set_texture(rl->load("res://asset/itemContainer.png"));
		container->add_child(bg);
		
		lbl = memnew(Label);
		lbl->add_theme_font_size_override("font_size", 12);
		lbl->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
		lbl->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
		lbl->set_position(Vector2(-16, -16));
		lbl->set_size(Vector2(32, 32));
		container->add_child(lbl);
		
		get_parent()->add_child(container);
	}
	
	if (val.get_type() == Variant::INT || val.get_type() == Variant::FLOAT) {
		float f_val = val;
		if (f_val == Math::round(f_val)) {
			lbl->set_text(String::num(int(f_val)));
		} else {
			lbl->set_text(String::num(f_val));
		}
	} else {
		lbl->set_text(val);
	}
	
	return container;
}

void FastWireSystem::recycle_item_visual(Node2D* vis) {
	if (vis) {
		vis->hide();
		item_visual_pool.push_back(vis);
	}
}

void FastWireSystem::notify_grid_changed(Vector2i t) {
	if (wire_grid.count(t)) {
		trigger_cache_update(wire_grid[t]);
	}
	for (int i = 0; i < WIRE_DIRS.size(); i++) {
		Vector2i neighbor = t + WIRE_DIRS[i];
		if (wire_grid.count(neighbor)) {
			trigger_cache_update(wire_grid[neighbor]);
		}
	}
}

PackedVector2Array FastWireSystem::get_bezier_points(Vector2 p0, Vector2 p1, Vector2 p2, int segments) {
	PackedVector2Array pts;
	Vector2 prev_pt = p0.lerp(p1, 0.0f).lerp(p1.lerp(p2, 0.0f), 0.0f);
	for (int i = 1; i <= segments; i++) {
		float t = float(i) / float(segments);
		Vector2 q0 = p0.lerp(p1, t);
		Vector2 q1 = p1.lerp(p2, t);
		Vector2 current_pt = q0.lerp(q1, t);
		pts.push_back(prev_pt);
		pts.push_back(current_pt);
		prev_pt = current_pt;
	}
	return pts;
}

void FastWireSystem::trigger_cache_update(WireCell& cell) {
	if (cell.type == WireType::CLASSIC) {
		cell.cache_in_dir = cell.dir;
		for (int i = 0; i < WIRE_DIRS.size(); i++) {
			Vector2i test_dir = WIRE_DIRS[i];
			Vector2i neighbor = cell.tile - test_dir;
			if (wire_grid.count(neighbor)) {
				const WireCell& nw = wire_grid[neighbor];
				bool connected = false;
				if (nw.type == WireType::CLASSIC && nw.dir == test_dir) connected = true;
				else if (nw.type == WireType::JUNCTION) {
					for (size_t j = 0; j < nw.dirs.size(); j++) {
						if (nw.dirs[j] == test_dir) { connected = true; break; }
					}
				}
				if (connected) {
					cell.cache_in_dir = test_dir;
					break;
				}
			}
		}
		Vector2 center = wire_tile_to_pos(cell.tile);
		Vector2 p0 = center - Vector2(cell.cache_in_dir) * 8.0f;
		Vector2 p1 = center;
		Vector2 p2 = center + Vector2(cell.dir) * 8.0f;
		cell.cache_pts = get_bezier_points(p0, p1, p2, 8);
		cell.cache_arrow_pos = p0 * 0.25f + p1 * 0.5f + p2 * 0.25f;
		cell.cache_arrow_angle = (p2 - p0).angle();
	}
}

void FastWireSystem::_draw() {
	Camera2D* camera = get_viewport()->get_camera_2d();
	std::vector<Vector2i> on_screen;
	
	if (camera) {
		Vector2 vp_size = get_viewport_rect().get_size() / camera->get_zoom();
		Vector2 center_pos = camera->get_screen_center_position();
		
		Vector2 min_pos = center_pos - vp_size / 2.0f - Vector2(32, 32);
		Vector2 max_pos = center_pos + vp_size / 2.0f + Vector2(64, 64);
		Rect2 view_rect(min_pos, max_pos - min_pos);
		
		for (const auto& pair : wire_grid) {
			Vector2 center(pair.first.x * 16.0f + 8.0f, pair.first.y * 16.0f + 8.0f);
			if (view_rect.has_point(center)) {
				on_screen.push_back(pair.first);
			}
		}
	} else {
		for (const auto& pair : wire_grid) {
			on_screen.push_back(pair.first);
		}
	}
	
	float time = Time::get_singleton()->get_ticks_msec() / 1000.0f;
	
	// Draw Outlines
	for (Vector2i t : on_screen) {
		WireCell& cell = wire_grid[t];
		Vector2 center = wire_tile_to_pos(t);
		if (cell.type == WireType::CLASSIC) {
			if (debug_mode) {
				draw_rect(Rect2(center - Vector2(4, 4), Vector2(8, 8)), Color(0.2, 0.9, 1.0));
			} else {
				draw_multiline(cell.cache_pts, outline_color, outline_width);
			}
		} else if (cell.type == WireType::JUNCTION) {
			draw_rect(Rect2(center - Vector2(8, 8), Vector2(16, 16)), outline_color);
		}
	}
	
	// Draw Fill
	for (Vector2i t : on_screen) {
		WireCell& cell = wire_grid[t];
		Vector2 center = wire_tile_to_pos(t);
		if (cell.type == WireType::CLASSIC) {
			if (debug_mode) {
				draw_line(center, center + Vector2(cell.dir) * 8.0f, Color(1, 1, 1), 2.0f);
			} else {
				float phase = (t.x * cell.dir.x + t.y * cell.dir.y) * 1.5f - time * 10.0f;
				float flash = Math::max(0.0f, Math::sin(phase));
				Color flash_color = wire_color.lerp(Color(1,1,1), flash * 0.7f);
				draw_multiline(cell.cache_pts, flash_color, wire_width);
			}
		} else if (cell.type == WireType::JUNCTION) {
			Vector2i d = cell.dirs.empty() ? Vector2i(1, 0) : cell.dirs[0];
			float phase = (t.x * d.x + t.y * d.y) * 1.5f - time * 10.0f;
			float flash = Math::max(0.0f, Math::sin(phase));
			Color flash_color = wire_color.lerp(Color(1,1,1), flash * 0.7f);
			draw_rect(Rect2(center - Vector2(6, 6), Vector2(12, 12)), flash_color);
		}
	}
	
	// Draw Arrows
	if (!debug_mode) {
		for (Vector2i t : on_screen) {
			WireCell& cell = wire_grid[t];
			Vector2 center = wire_tile_to_pos(t);
			if (cell.type == WireType::CLASSIC) {
				float phase = (t.x * cell.dir.x + t.y * cell.dir.y) * 1.5f - time * 10.0f;
				float flash = Math::max(0.0f, Math::sin(phase));
				if (flash > 0.01f && arrow_tex.is_valid()) {
					Color arr_color = Color(1.0, 1.0, 1.0, flash);
					draw_set_transform(cell.cache_arrow_pos, cell.cache_arrow_angle, Vector2(1,1));
					draw_texture_rect(arrow_tex, Rect2(-arrow_render_size/2.0f, arrow_render_size), false, arr_color);
					draw_set_transform(Vector2(), 0.0f, Vector2(1,1));
				}
			} else if (cell.type == WireType::JUNCTION) {
				Vector2i d = cell.dirs.empty() ? Vector2i(1, 0) : cell.dirs[0];
				float phase = (t.x * d.x + t.y * d.y) * 1.5f - time * 10.0f;
				float flash = Math::max(0.0f, Math::sin(phase));
				if (flash > 0.01f && arrow_tex.is_valid()) {
					Color arr_color = Color(1.0, 1.0, 1.0, flash);
					draw_set_transform(center, Vector2(d).angle(), Vector2(1,1));
					draw_texture_rect(arrow_tex, Rect2(-arrow_render_size/2.0f, arrow_render_size), false, arr_color);
					draw_set_transform(Vector2(), 0.0f, Vector2(1,1));
				}
			}
		}
	}
	
	// Draw Preview
	if (preview_points.size() > 0) {
		for (int i = 0; i < preview_points.size(); i++) {
			Vector2i p = preview_points[i];
			Vector2 center = wire_tile_to_pos(p);
			
			Vector2i out_dir = WIRE_DIRS[current_wire_dir_index];
			if (preview_points.size() > 1) {
				if (i < preview_points.size() - 1) {
					out_dir = Vector2i(preview_points[i+1]) - p;
				} else if (i > 0) {
					out_dir = p - Vector2i(preview_points[i-1]);
				}
			}
			Vector2i in_dir = out_dir;
			if (i > 0) in_dir = p - Vector2i(preview_points[i-1]);
			
			bool is_blocked = false;
			if (grid_system) {
				Variant cell_var = grid_system->call("pos_to_cell", center);
				if (cell_var.get_type() == Variant::VECTOR2I) {
					Vector2i cell = cell_var;
					Dictionary grid_data = grid_system->get("grid_data");
					if (grid_data.has(cell)) {
						Dictionary cell_dict = grid_data[cell];
						is_blocked = cell_dict.get("is_building", true);
					}
				}
			}
			
			if (debug_mode) {
				Color c = is_blocked ? Color(1.0, 0.0, 0.0, 0.5) : Color(0.2, 0.9, 1.0, 0.5);
				draw_rect(Rect2(center - Vector2(4, 4), Vector2(8, 8)), c);
				draw_line(center, center + Vector2(out_dir) * 8.0f, Color(1, 1, 1, 0.5), 2.0f);
			} else {
				Color c_wire = is_blocked ? Color(1.0, 0.2, 0.2, 0.7) : Color(0.3, 0.8, 1.0, 0.7);
				Color c_outline = is_blocked ? Color(0.1, 0.0, 0.0, 0.5) : Color(0.1, 0.1, 0.15, 0.7);
				Vector2 p0 = center - Vector2(in_dir) * 8.0f;
				Vector2 p1 = center;
				Vector2 p2 = center + Vector2(out_dir) * 8.0f;
				PackedVector2Array pts = get_bezier_points(p0, p1, p2, 8);
				draw_multiline(pts, c_outline, outline_width);
				draw_multiline(pts, c_wire, wire_width);
			}
		}
	}
}

void FastWireSystem::process_items(float delta, float game_speed) {
	float speed = 1500.0f * game_speed;
	
	// Update progress
	for (const auto& pair : active_wires) {
		Vector2i t = pair.first;
		if (wire_grid.count(t)) {
			WireCell& cell = wire_grid[t];
			if (cell.type == WireType::CLASSIC) {
				if (cell.has_item) {
					cell.item.progress += speed * delta;
					if (cell.item.progress > 16.0f) cell.item.progress = 16.0f;
				}
			} else if (cell.type == WireType::JUNCTION) {
				for (auto& itm : cell.items) {
					itm.progress += speed * delta;
					if (itm.progress > 16.0f) itm.progress = 16.0f;
				}
			}
		}
	}
	
	// Push items
	bool moved_any = true;
	int max_cascades = 16;
	while (moved_any && max_cascades > 0) {
		moved_any = false;
		max_cascades--;
		
		std::vector<Vector2i> keys;
		for (const auto& pair : active_wires) keys.push_back(pair.first);
		
		for (Vector2i t : keys) {
			if (wire_grid.count(t)) {
				moved_any = push_items_for_cell(wire_grid[t], moved_any);
			}
		}
	}
	
	// Interpolate
	for (const auto& pair : active_wires) {
		Vector2i t = pair.first;
		if (wire_grid.count(t)) {
			interpolate_items_for_cell(wire_grid[t]);
		}
	}
}

bool FastWireSystem::push_items_for_cell(WireCell& cell, bool moved_any) {
	if (cell.type == WireType::CLASSIC) {
		if (cell.has_item && cell.item.progress >= 16.0f) {
			Vector2i next_t = cell.tile + cell.dir;
			Object* b = get_building_at(next_t);
			if (b) {
				bool can_receive = true;
				if (b->has_method("can_receive")) can_receive = b->call("can_receive", cell.tile);
				if (can_receive) {
					if (b->has_method("receive")) b->call("receive", cell.item.value, cell.tile);
					if (cell.item.visual) recycle_item_visual(cell.item.visual);
					cell.has_item = false;
					active_wires.erase(cell.tile);
					return true;
				}
			} else {
				if (wire_grid.count(next_t)) {
					WireCell& next_w = wire_grid[next_t];
					Dictionary dict;
					dict["value"] = cell.item.value;
					dict["visual"] = cell.item.visual;
					if (accept_item(next_t, dict, cell.dir)) {
						cell.has_item = false;
						active_wires.erase(cell.tile);
						return true;
					}
				}
			}
		}
		return moved_any;
	} else if (cell.type == WireType::JUNCTION) {
		std::vector<size_t> items_to_remove;
		bool locally_moved = false;
		
		for (size_t i = 0; i < cell.items.size(); i++) {
			WireItem& itm = cell.items[i];
			if (itm.progress >= 16.0f) {
				std::vector<Vector2i> dirs_to_try;
				int start_idx = 0;
				Vector2i mdir = itm.move_dir;
				
				bool mdir_in_dirs = false;
				for (auto d : cell.dirs) if (d == mdir) mdir_in_dirs = true;
				
				if (mdir != Vector2i() && mdir_in_dirs) {
					dirs_to_try.push_back(mdir);
				} else if (cell.dirs.size() > 0) {
					start_idx = cell.rr_idx;
					dirs_to_try = cell.dirs;
				}
				
				for (size_t j = 0; j < dirs_to_try.size(); j++) {
					int idx = (start_idx + j) % dirs_to_try.size();
					Vector2i try_dir = dirs_to_try[idx];
					Vector2i next_t = cell.tile + try_dir;
					
					Object* b = get_building_at(next_t);
					if (b) {
						bool can_receive = true;
						if (b->has_method("can_receive")) can_receive = b->call("can_receive", cell.tile);
						if (can_receive) {
							if (b->has_method("receive")) b->call("receive", itm.value, cell.tile);
							if (itm.visual) recycle_item_visual(itm.visual);
							items_to_remove.push_back(i);
							locally_moved = true;
							if (dirs_to_try.size() > 1) cell.rr_idx = (idx + 1) % dirs_to_try.size();
							break;
						}
					} else {
						if (wire_grid.count(next_t)) {
							Dictionary dict;
							dict["value"] = itm.value;
							dict["visual"] = itm.visual;
							if (accept_item(next_t, dict, try_dir)) {
								items_to_remove.push_back(i);
								locally_moved = true;
								if (dirs_to_try.size() > 1) cell.rr_idx = (idx + 1) % dirs_to_try.size();
								break;
							}
						}
					}
				}
			}
		}
		
		for (int i = (int)items_to_remove.size() - 1; i >= 0; i--) {
			cell.items.erase(cell.items.begin() + items_to_remove[i]);
		}
		
		if (cell.items.empty()) {
			active_wires.erase(cell.tile);
		}
		return moved_any || locally_moved;
	}
	return moved_any;
}

bool FastWireSystem::accept_item(Vector2i tile, const Dictionary& itm_dict, Vector2i from_dir) {
	if (!wire_grid.count(tile)) return false;
	WireCell& cell = wire_grid[tile];
	
	if (cell.type == WireType::CLASSIC) {
		if (!cell.has_item) {
			cell.has_item = true;
			cell.item.value = itm_dict.get("value", Variant());
			cell.item.visual = Object::cast_to<Node2D>(itm_dict.get("visual", Variant()).operator Object *());
			cell.item.progress = 0.0f;
			cell.item.move_dir = from_dir;
			active_wires[tile] = true;
			return true;
		}
	} else if (cell.type == WireType::JUNCTION) {
		int items_in_dir = 0;
		for (auto& j_itm : cell.items) {
			if (j_itm.move_dir == from_dir) items_in_dir++;
		}
		if (items_in_dir < 1) {
			WireItem new_itm;
			new_itm.value = itm_dict.get("value", Variant());
			new_itm.visual = Object::cast_to<Node2D>(itm_dict.get("visual", Variant()).operator Object *());
			new_itm.progress = 0.0f;
			new_itm.move_dir = from_dir;
			cell.items.push_back(new_itm);
			active_wires[tile] = true;
			return true;
		}
	}
	return false;
}

void FastWireSystem::interpolate_items_for_cell(WireCell& cell) {
	Vector2 center = wire_tile_to_pos(cell.tile);
	if (cell.type == WireType::CLASSIC) {
		if (cell.has_item && cell.item.visual) {
			Vector2i in_dir = cell.item.move_dir;
			if (in_dir == Vector2i()) in_dir = cell.dir;
			Vector2 p0 = center - Vector2(in_dir) * 8.0f;
			Vector2 p1 = center;
			Vector2 p2 = center + Vector2(cell.dir) * 8.0f;
			float frac = cell.item.progress / 16.0f;
			Vector2 q0 = p0.lerp(p1, frac);
			Vector2 q1 = p1.lerp(p2, frac);
			cell.item.visual->set_global_position(q0.lerp(q1, frac));
		}
	} else if (cell.type == WireType::JUNCTION) {
		for (auto& itm : cell.items) {
			if (itm.visual) {
				Vector2i anim_dir = itm.move_dir;
				if (anim_dir == Vector2i()) anim_dir = Vector2i(1, 0);
				Vector2 start = center - Vector2(anim_dir) * 8.0f;
				Vector2 end = center + Vector2(anim_dir) * 8.0f;
				float frac = itm.progress / 16.0f;
				itm.visual->set_global_position(start.lerp(end, frac));
			}
		}
	}
}

bool FastWireSystem::has_wire(Vector2i pos) { return wire_grid.count(pos) > 0; }
String FastWireSystem::get_wire_type(Vector2i pos) {
	if (!wire_grid.count(pos)) return "";
	if (wire_grid[pos].type == WireType::CLASSIC) return "wire";
	if (wire_grid[pos].type == WireType::JUNCTION) return "junction";
	return "";
}
Vector2i FastWireSystem::get_wire_dir(Vector2i pos) {
	if (wire_grid.count(pos) && wire_grid[pos].type == WireType::CLASSIC) return wire_grid[pos].dir;
	return Vector2i();
}
Array FastWireSystem::get_junction_dirs(Vector2i pos) {
	Array arr;
	if (wire_grid.count(pos) && wire_grid[pos].type == WireType::JUNCTION) {
		for (auto d : wire_grid[pos].dirs) arr.push_back(d);
	}
	return arr;
}
void FastWireSystem::set_wire(Vector2i pos, Vector2i dir) {
	WireCell cell;
	cell.type = WireType::CLASSIC;
	cell.tile = pos;
	cell.dir = dir;
	wire_grid[pos] = cell;
	notify_grid_changed(pos);
}
void FastWireSystem::set_junction(Vector2i pos, Array dirs) {
	WireCell cell;
	cell.type = WireType::JUNCTION;
	cell.tile = pos;
	for (int i = 0; i < dirs.size(); i++) cell.dirs.push_back(dirs[i]);
	wire_grid[pos] = cell;
	notify_grid_changed(pos);
}
void FastWireSystem::remove_wire(Vector2i pos) {
	if (wire_grid.count(pos)) {
		WireCell& cell = wire_grid[pos];
		if (cell.type == WireType::CLASSIC && cell.has_item && cell.item.visual) recycle_item_visual(cell.item.visual);
		if (cell.type == WireType::JUNCTION) {
			for (auto& itm : cell.items) if (itm.visual) recycle_item_visual(itm.visual);
		}
		wire_grid.erase(pos);
		active_wires.erase(pos);
		notify_grid_changed(pos);
		queue_redraw();
	}
}
void FastWireSystem::add_junction_dir(Vector2i pos, Vector2i dir) {
	if (wire_grid.count(pos) && wire_grid[pos].type == WireType::JUNCTION) {
		bool exists = false;
		for (auto d : wire_grid[pos].dirs) if (d == dir) exists = true;
		if (!exists) {
			wire_grid[pos].dirs.push_back(dir);
			notify_grid_changed(pos);
		}
	}
}

Dictionary FastWireSystem::get_wire_item(Vector2i pos) {
	Dictionary dict;
	if (wire_grid.count(pos)) {
		if (wire_grid[pos].type == WireType::CLASSIC && wire_grid[pos].has_item) {
			dict["value"] = wire_grid[pos].item.value;
			dict["visual"] = wire_grid[pos].item.visual;
		}
	}
	return dict;
}
