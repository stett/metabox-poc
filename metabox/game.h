#ifndef _GAME_H_
#define _GAME_H_

#include <memory>
#include <list>
#include <vector>
#include <string>
#include <math.h>
#include <SFML/Graphics.hpp>
#include "Box.h"
#include "Player.h"
#include "View.h"

using std::shared_ptr;
using std::unique_ptr;
using std::list;
using std::vector;
using std::string;
using std::to_string;
using std::map;

struct game {

public:
	// Types
	enum Mode { Play, Edit, Quit };

public:
	void setup();
	void teardown();
	void run();

private:
	// Private game functions
	void set_mode(Mode new_mode);
	void step(float dt);
	void draw();
	void get_view_transforms(sf::RenderStates& states);
	void render_game();
	void render_editor();
	void render_box(shared_ptr<Box> box);
	void render_child(shared_ptr<Box> parent, shared_ptr<Box> box);
	void render_box_fg(shared_ptr<Box> box);
	void get_box_shader(shared_ptr<Box> box, sf::RenderStates& states, bool door_shader = true, bool entropy_shader = true);
	shared_ptr<Box> add_box(shared_ptr<Box> parent = 0, int sx = 0, int sy = 0, bool recursive = false);
	void add_box_hull(shared_ptr<Box> box, shared_ptr<b2World> world, float size, int sx, int sy);
	void make_metabox(shared_ptr<Box> box, int sx, int sy);
	void add_block(shared_ptr<Box> parent, int sx, int sy);
	void assign_box_texture(shared_ptr<Box> box);
	void set_box_door(shared_ptr<Box> box, BoxFace face, int i, bool open = false);
	void set_box_door(shared_ptr<Box> box, BoxFace face, Slot* slot, bool open);
	void open_box_door(shared_ptr<Box> box, BoxFace, bool open);
	void generate_box_edges(shared_ptr<Box> box);
	void generate_world_edges(shared_ptr<Box> box);
	void set_player_container(shared_ptr<Box> box, b2Vec2 position);
	void set_player_container(shared_ptr<Box> box, b2Vec2 position, b2Vec2 velocity);
	void center_view_on_slot(int sx, int sy, bool target = true);
	void center_view_on_parent_slot(int sx, int sy, bool target = true);
	void set_window_size(int w, int h);
	void find_door_shingle(shared_ptr<BoxDoor> door);
	void find_door_adjacencies(shared_ptr<Box> box);
	void find_door_adjacency(shared_ptr<BoxDoor> door);
	void set_door_adjacency(shared_ptr<BoxDoor> door0, shared_ptr<BoxDoor> door1);
	Slot* get_adjacent_slot(Slot* slot, BoxFace face);

	// Private game members
	Mode mode = Mode::Play;
	shared_ptr<b2World> outer_world;
	shared_ptr<Box> root_box;
	list<shared_ptr<Box>> boxes;
	list<shared_ptr<sf::RenderTexture>> box_textures;
	list<shared_ptr<sf::RenderTexture>> unused_box_textures;
	unique_ptr<sf::RenderWindow> window;
	View view;
	sf::Font font;
	sf::Texture box_bg;
	sf::Texture box_fg;
	sf::Texture grid;
	sf::Texture player_tex;
	sf::Texture block_tex;
	sf::Shader meta_box_shader;
	sf::Shader meta_door_shader;
	int next_box_id;
	float fps;
	Player player;
	shared_ptr<BoxDoor> nearest_door;
	BoxFace nearest_door_face;
};

#endif