#include "game.h"
#include "settings.h"
#include "Box.h"
#include "Player.h"
#include "View.h"
#include "vec2f.h"
#include <SFML/OpenGL.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/System/Clock.hpp>
#include <memory>
#include <list>
#include <vector>
#include <string>
#include <math.h>
using namespace std;

namespace game {

	// Private game members
	bool running = true;
	shared_ptr<Box> root_box;
	list<shared_ptr<Box>> boxes;
	list<shared_ptr<sf::RenderTexture>> box_textures;
	list<shared_ptr<sf::RenderTexture>> unused_box_textures;
	unique_ptr<sf::RenderWindow> window;
	View view;
	sf::Font font;
	sf::Texture box_bg;
	sf::Texture grid;
	sf::Texture player_tex;
	sf::Texture block_tex;
	sf::Shader open_meta_door_shader;
	int next_box_id;
	float fps;
	Player player;
	shared_ptr<BoxDoor> nearest_door;
	BoxFace nearest_door_face;
	
	// Private game functions
	void step(float dt);
	void draw();
	void get_view_transforms(sf::RenderStates& states);
	void render_game();
	void render_editor();
	void render_box(shared_ptr<Box> box);
	void get_box_shader(shared_ptr<Box> box, sf::RenderStates& states);
	shared_ptr<Box> add_box(shared_ptr<Box> parent = 0, int sx = 0, int sy = 0);
	void add_block(shared_ptr<Box> parent, int sx, int sy);
	void assign_box_texture(shared_ptr<Box> box);
	void set_box_door(shared_ptr<Box> box, BoxFace face, int i, bool open = false);
	void open_box_door(shared_ptr<Box> box, BoxFace, bool open);
	void generate_box_edges(shared_ptr<Box> box);
	void generate_world_edges(shared_ptr<Box> box);
	void set_player_container(shared_ptr<Box> box, b2Vec2 position);
	void set_player_container(shared_ptr<Box> box, b2Vec2 position, b2Vec2 velocity);
	void center_view_on_slot(int sx, int sy, bool target = true);
};

void game::setup() {
	next_box_id = 0;

	// TEMP
	//center_view_on_slot(7, 7, false);

	// Add a bunch of boxes
	auto a = add_box();
	auto b = add_box(a, 2, 2);
	auto c = add_box(a, 4, 2);
	auto d = add_box(b, 2, 1);
	auto e = add_box(c, 3, 3);
	auto f = add_box(e, 3, 3);
	root_box = a;

	//a->world->SetGravity(b2Vec2(0, 0));

	set_box_door(b, Right, 6);
	set_box_door(b, Left, 0);
	set_box_door(b, Top, 2);
	set_box_door(c, Left, 3);
	set_box_door(c, Top, 3);
	set_box_door(d, Right, 4);
	set_box_door(e, Top, 3);
	set_box_door(f, Top, 4);

	for (int i = 0; i < 7; i++) {
		add_block(a, i, 6);
		add_block(a, i, 5);
		add_block(a, i, 4);
		add_block(a, i, 3);
	}

	add_block(b, 0, 1);
	add_block(b, 1, 1);
	add_block(b, 0, 2);
	add_block(b, 1, 2);
	add_block(b, 2, 2);
	add_block(b, 3, 2);

	add_block(d, 6, 5);
	add_block(d, 6, 6);

	// Add the player to the first box and give it a body
	set_player_container(b, b2Vec2(4, 3));

	// Create the main window
	window = unique_ptr<sf::RenderWindow>(new sf::RenderWindow(sf::VideoMode(BOX_RENDER_SIZE, BOX_RENDER_SIZE), "Metabox Surfaces - Proof Of Concept", sf::Style::Default));
	window->setActive(true);

	// Create a graphical text to display
	font.loadFromFile("fonts/consola.ttf");

	// Load textures
	box_bg.loadFromFile("graphics/3grid.png");
	grid.loadFromFile("graphics/7grid.png");
	player_tex.loadFromFile("graphics/player.png");
	block_tex.loadFromFile("graphics/block.png");

	// Load the open-meta-door shader
	open_meta_door_shader.loadFromFile("open_meta_door.vert", "open_meta_door.frag");
}

void game::run() {

	float dt_min = 1.f / 60.f;
	float dt_accum = 0;
	sf::Clock clock;
	sf::Time t0 = clock.getElapsedTime();
	sf::Time t1;

	while (running) {
		
		t1 = clock.getElapsedTime();
		float dt = (t1 - t0).asSeconds();
		fps = 1.f / dt;
		t0 = t1;
		dt_accum += dt;
		while (dt_accum >= dt_min) {
			dt_accum -= dt_min;
			step(dt_min);
		}
		
		draw();

		// Clear old forces
		for (auto box : boxes) {
			box->world->ClearForces();
		}
	};
}

void game::step(float dt) {

	// Process events
	sf::Event event;
	while (window->pollEvent(event))
	{
		// Close window : exit
		if (event.type == sf::Event::Closed) window->close();

		// Resize window : change viewport
		if (event.type == sf::Event::Resized) {
			// TODO
		}

		//
		if (event.type == sf::Event::KeyPressed) {

			if (event.key.code == sf::Keyboard::Escape) window->close();

			if (event.key.code == sf::Keyboard::Up || event.key.code == sf::Keyboard::W) {
				player.body->ApplyForceToCenter(b2Vec2(0, -250), true);
			}
		}
	}

	// Apply forces to the player based on keyboard input
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) || sf::Keyboard::isKeyPressed(sf::Keyboard::D))
		player.body->ApplyForceToCenter(b2Vec2(12, 0), true);
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) || sf::Keyboard::isKeyPressed(sf::Keyboard::A))
		player.body->ApplyForceToCenter(b2Vec2(-12, 0), true);
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down) || sf::Keyboard::isKeyPressed(sf::Keyboard::S))
		player.body->ApplyForceToCenter(b2Vec2(0, 5), true);


	// Update all boxes
	for (auto box : boxes) {
		box->world->Step(dt, 6, 2);

		// Update the box's phsyics body
		if (box->body) {
			auto pos = box->body->GetPosition();
			box->sx = (int)(pos.x * (float)BOX_SLOTS / (float)BOX_PHYSICAL_SIZE);
			box->sy = (int)(pos.y * (float)BOX_SLOTS / (float)BOX_PHYSICAL_SIZE);

			if (box->state == Gridded) {
				auto target_pos = b2Vec2((box->target_sx + .5f) * BOX_METERS_PER_SLOT, (box->target_sy + .5f) * BOX_METERS_PER_SLOT);
				auto diff = target_pos - pos;
				box->body->SetLinearVelocity(b2Vec2(diff.x * 5, diff.y * 5));
			}
		}

		// Update door transitions
		for (auto door : box->doors) {
			if (door) {
				if (door->open) {
					if (door->t < 1)
						door->t += 4 * dt;
					else door->t = 1;
				} else {
					if (door->t > 0)
						door->t -= 4 * dt;
					else door->t = 0;
				}
			}
		}
	}

	//
	if (nearest_door && nearest_door->open)
		open_box_door(nearest_door->parent, nearest_door_face, false);

	//
	nearest_door = 0;
	float max_door_dist = 2;
	float nearest_door_dist = max_door_dist;

	// Close/open active metabox doors
	for (int face = 0; face < 4; face++) {
		auto door = player.container->doors[face];
		if (door) {

			// Get distance from player to door
			auto door_pos = sf::Vector2f(door->sx + .5f, door->sy + .5f) * BOX_METERS_PER_SLOT;
			auto player_pos = player.body->GetPosition();
			auto diff = vec2f(door_pos) - vec2f(player_pos);
			float dist = diff.length();

			// If player is too far away, close door
			/*
			if (door->open) {
				if (dist > max_door_dist)
					open_box_door(player.container, (BoxFace)face, false);
			}
			*/
			
			// If this is the closest door thus far, save it
			if (dist < nearest_door_dist) {
				nearest_door = door;
				nearest_door_dist = dist;
				nearest_door_face = (BoxFace)face;
			}
		}
	}

	// Close/open sub metabox doors
	for (auto box : player.container->children) {
		for (int face = 0; face < 4; face++) {
			auto door = box->doors[face];
			if (door) {

				// Get distance from player to door
				vec2f player_pos = vec2f(player.body->GetPosition());
				vec2f door_pos = vec2f(box->body->GetPosition())
							   + vec2f(face == 1 ?  1 : (face == 3 ? -1 : 0),
									   face == 0 ? -1 : (face == 2 ?  1 : 0)) * .5f;
				vec2f diff = door_pos - player_pos;
				float dist = diff.length();

				// If player is too far away, close door
				/*
				if (door->open) {
					if (dist > max_door_dist)
						open_box_door(box, (BoxFace)face, false);
				}
				*/
				
				// If this is the closest door thus far, save it
				if (dist < nearest_door_dist) {
					nearest_door = door;
					nearest_door_dist = dist;
					nearest_door_face = (BoxFace)face;
				}
			}
		}
	}

	// If we found a close enough door, open it!
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && nearest_door)
		open_box_door(nearest_door->parent, nearest_door_face, true);

	// Process player meta-transitions
	// TODO: This *should* transfer to the ADJACENT box, not just always to the parent.
	//       Once the adjacency system is up we can do it that wasy. Doing it the right
	//		 way will prevent the need to separate entering/exiting metaboxes, and will
	//		 also automatically facilitate "lateral" transitions in the box-tree.
	{
		bool player_transfered = false;

		// If the player has wandered out of a metadoor,
		// transfer them to the parent box.
		auto player_pos = player.body->GetPosition();
		float max_pos = (float)BOX_PHYSICAL_SIZE;
		if (player_pos.x < 0 || player_pos.y < 0 || player_pos.x > max_pos || player_pos.y > max_pos) {

			if (player.container->parent) {

				// Calculate the new player position
				float off = .5f * (float)BOX_PHYSICAL_SIZE / (float)BOX_SLOTS;
				player_pos -= b2Vec2(.5f * BOX_PHYSICAL_SIZE, .5f * BOX_PHYSICAL_SIZE);
				player_pos.x /= (float)BOX_SLOTS;
				player_pos.y /= (float)BOX_SLOTS;
				player_pos += player.container->body->GetPosition();

				// Set the player container
				set_player_container(player.container->parent, player_pos, player.body->GetLinearVelocity());
				player_transfered = true;
			}
		}

		// If the player has wandered into a sub-meta door,
		// transfer them into the child box.
		for (auto child : player.container->children) {

			shared_ptr<BoxDoor> door = 0;
			for (int i = 0; i < 4; i++) {
				if (child->doors[i] && child->doors[i]->open) {
					door = child->doors[i];
					break;
				}
			}
			if (!door) continue;

			if (player_transfered) break;
			for (b2Fixture* fixture = child->body->GetFixtureList(); fixture; fixture = fixture->GetNext()) {
				if (fixture->GetType() != b2Shape::Type::e_polygon) continue;
				if (fixture->GetShape()->TestPoint(child->body->GetTransform(), player.body->GetPosition())) {

					player_pos -= child->body->GetPosition();
					player_pos += b2Vec2(door->sx * BOX_PHYSICAL_SIZE / BOX_SLOTS, door->sy * BOX_PHYSICAL_SIZE / BOX_SLOTS);
					player_pos += b2Vec2(.5f * BOX_PHYSICAL_SIZE / BOX_SLOTS, .5f * BOX_PHYSICAL_SIZE / BOX_SLOTS);

					set_player_container(child, player_pos, player.body->GetLinearVelocity());

					break;
					player_transfered = true;
				}
			}
		}
	}

	// Set view target parameters
	for (auto door : player.container->doors) {
		if (door && door->open) {
			view.tscale = .5;
			break;
		}
		view.tscale = 1;
	}

	// Update the view active parameters
	view.x = view.x + (view.tx - view.x) * 3 * dt;
	view.y = view.y + (view.ty - view.y) * 3 * dt;
	view.scale = view.scale + (view.tscale - view.scale) * 4 * dt;
	
	// Stop running when the window is closed
	running = window->isOpen();
}

void game::draw() {

	// Clear screen
	window->clear(sf::Color(100, 100, 100));

	//
	//render_editor();
	render_game();

	// Draw the FPS
	sf::Text text(to_string((int)fps), font, 12);
	text.setPosition(sf::Vector2f(2, 2));
	text.setColor(sf::Color(255, 0, 0, 255));
	window->draw(text);


	// Update the window
	window->display();
}

void game::get_view_transforms(sf::RenderStates& states) {
	states.transform.translate(sf::Vector2f(window->getSize().x * .5f, window->getSize().x * .5f))
					.scale(sf::Vector2f(view.scale, view.scale))
					.translate(sf::Vector2f(view.x, view.y))
					.translate(-sf::Vector2f(window->getSize().x * .5f, window->getSize().x * .5f));
}

void game::render_game() {

	// Find the active box
	auto active_box = player.container;
	auto active_parent = active_box->parent;

	// If the active box has a parent, 
	if (active_parent) {
		render_box(active_parent);
		sf::Sprite sprite(active_parent->texture->getTexture());
		sf::RenderStates states;

		// Apply view transformations
		get_view_transforms(states);

		vec2f pos = vec2f(active_box->sx, active_box->sy) * PIXELS_PER_METER * BOX_METERS_PER_SLOT;
		states.transform.scale(sf::Vector2f(BOX_SLOTS, BOX_SLOTS))
			  .translate(-pos.toVector2f());

		window->draw(sprite, states);
	}

	// Render the active box (& its visible children) and get its sprite
	if (!active_parent) render_box(active_box);
	sf::Sprite sprite(active_box->texture->getTexture());
	sf::RenderStates states;

	// Apply view transformations
	get_view_transforms(states);

	// Apply the door shader
	get_box_shader(active_box, states);

	// Draw the active box
	window->draw(sprite, states);
}

void game::render_editor() {

	// Render the boxes to their textures
	render_box(root_box);

	// Draw all the box textures & calculate thier positions
	sf::Vector2i ipos(0, 0);
	map<int, sf::Vector2f> box_positions;
	int iw = 3;
	float pad = 26;
	for (auto box : boxes) {
		if (!box->texture) continue;

		// Convert the rendered box texture to a sprite and draw it to the screen
		sf::Sprite sprite(box->texture->getTexture());
		sf::Vector2f pos(pad + ipos.x * (BOX_RENDER_SIZE + pad), pad + ipos.y * (BOX_RENDER_SIZE + pad));
		box_positions[box->id] = pos;
		sprite.setPosition(pos);
		sf::RenderStates states;
		get_box_shader(box, states);
		window->draw(sprite, states);

		// Calculate the next position indices
		ipos.x++;
		if (ipos.x >= iw) {
			ipos.x = 0;
			ipos.y++;
		}
	}

	// Render box overlay data
	for (auto box : boxes) {
		if (!box->texture) continue;

		// Get the position of this box on the gui
		auto box_position = box_positions[box->id];

		// Draw grid
		for (int i = 0; i < BOX_SLOTS; i++) {
			sf::Transform transform;
			transform.translate(box_position);
			sf::Vertex h_line[2];
			sf::Vertex v_line[2];
			v_line[0].position.x = h_line[0].position.y =
				v_line[1].position.x = h_line[1].position.y = i * (float)BOX_RENDER_SIZE / (float)BOX_PHYSICAL_SIZE;
			v_line[0].position.y = h_line[0].position.x = 0;
			v_line[1].position.y = h_line[1].position.x = BOX_RENDER_SIZE;
			h_line[0].color = h_line[1].color =
				v_line[0].color = v_line[1].color = sf::Color(255, 255, 255, 30);
			window->draw(h_line, 2, sf::PrimitiveType::Lines, sf::RenderStates(transform));
			window->draw(v_line, 2, sf::PrimitiveType::Lines, sf::RenderStates(transform));
		}

		// Draw the wireframes for the fixtures in this box's physics world
		for (b2Body* body = box->world->GetBodyList(); body; body = body->GetNext()) {

			// Compose the transformation for this body
			auto pos = body->GetPosition();
			auto ang = body->GetAngle();
			sf::Transform transform;
			transform.translate(box_position)
				.translate(sf::Vector2f(pos.x * PIXELS_PER_METER, pos.y * PIXELS_PER_METER))
				.rotate(ang * 180.f / 3.14159f);

			// Draw the fixtures
			for (b2Fixture* fixture = body->GetFixtureList(); fixture; fixture = fixture->GetNext()) {

				// Get the vertices for this fixture
				sf::VertexArray verts;
				verts.setPrimitiveType(sf::PrimitiveType::LinesStrip);
				if (fixture->GetType() == b2Shape::Type::e_polygon) {
					auto shape = (b2PolygonShape*)fixture->GetShape();
					int num_verts = shape->GetVertexCount();
					for (int i_vert = 0; i_vert < num_verts; i_vert++) {
						auto b2_vert = shape->GetVertex(i_vert);
						sf::Vertex vert(sf::Vector2f(b2_vert.x * PIXELS_PER_METER, b2_vert.y * PIXELS_PER_METER));
						vert.color = sf::Color(255, 255, 255, 150);
						verts.append(vert);
					}
					verts.append(verts[0]);
				}
				else if (fixture->GetType() == b2Shape::Type::e_chain) {
					auto shape = (b2ChainShape*)fixture->GetShape();
					int num_edges = shape->GetChildCount();
					for (int i_edge = 0; i_edge < num_edges; i_edge++) {
						b2EdgeShape edge;
						shape->GetChildEdge(&edge, i_edge);
						auto b2_vert = edge.m_vertex1;
						sf::Vertex vert(sf::Vector2f(b2_vert.x * PIXELS_PER_METER, b2_vert.y * PIXELS_PER_METER));
						vert.color = sf::Color::Green;
						verts.append(vert);
					}
					verts.append(verts[0]);
				}
				else if (fixture->GetType() == b2Shape::Type::e_edge) {
					auto shape = (b2EdgeShape*)fixture->GetShape();
					sf::Vertex a(sf::Vector2f(shape->m_vertex1.x * PIXELS_PER_METER, shape->m_vertex1.y * PIXELS_PER_METER));
					sf::Vertex b(sf::Vector2f(shape->m_vertex2.x * PIXELS_PER_METER, shape->m_vertex2.y * PIXELS_PER_METER));
					a.color = b.color = sf::Color::Green;
					verts.append(a);
					verts.append(b);
				}

				// Draw the vertices
				window->draw(verts, sf::RenderStates(transform));
			}
		}

		// Highlight doors if there are any
		for (int i = 0; i < 4; i++) {
			if (box->doors[i]) {
				sf::Transform transform;
				transform.translate(box_position);

				sf::Vector2f pos(
					box->doors[i]->sx * BOX_METERS_PER_SLOT * PIXELS_PER_METER,
					box->doors[i]->sy * BOX_METERS_PER_SLOT * PIXELS_PER_METER);
				sf::RectangleShape rect;
				rect.setPosition(pos);
				rect.setSize(sf::Vector2f(BOX_METERS_PER_SLOT * PIXELS_PER_METER, BOX_METERS_PER_SLOT * PIXELS_PER_METER));
				if (box->doors[i]->open) {
					rect.setOutlineColor(sf::Color::Blue);
					rect.setFillColor(sf::Color(0, 0, 255, 50));
				}
				else {
					rect.setOutlineColor(sf::Color::Red);
					rect.setFillColor(sf::Color(255, 0, 0, 50));
				}
				window->draw(rect, sf::RenderStates(transform));
			}
		}

		// Child data
		for (auto child : box->children) {
			sf::Transform transform;
			transform.translate(box_position);

			float size = (float)BOX_RENDER_SIZE / (float)BOX_SLOTS;

			// Slot highlight
			sf::RectangleShape rect;
			rect.setPosition(sf::Vector2f(child->sx, child->sy) * size - sf::Vector2f(2, 2));
			rect.setSize(sf::Vector2f(size + 4, size + 4));
			rect.setOutlineColor(sf::Color(150, 200, 255, 100));
			rect.setFillColor(sf::Color::Transparent);
			rect.setOutlineThickness(2);
			window->draw(rect, sf::RenderStates(transform));

			// Child box identifier string
			sf::Text text(to_string(child->id), font, 12);
			text.setPosition(sf::Vector2f(child->sx, child->sy) * size + sf::Vector2f(2, 2));
			text.setColor(sf::Color(255, 255, 255, 255));
			window->draw(text, sf::RenderStates(transform));
		}

		// Parent/child arrows
		if (box->parent) {
			sf::Vertex line[2];
			auto body_pos = box->body->GetPosition();
			line[0].position = box_position; box->parent->body->GetPosition();
			line[1].position = box_positions[box->parent->id] + sf::Vector2f(body_pos.x * PIXELS_PER_METER, body_pos.y * PIXELS_PER_METER);// +half;
			window->draw(line, 2, sf::PrimitiveType::Lines);
		}

		// Draw the box identifier string
		sf::Text text(to_string(box->id), font, 12);
		text.setPosition(box_position + sf::Vector2f(2, 2));
		text.setColor(sf::Color(255, 255, 255, 255));
		window->draw(text);
	}
}

void game::render_box(shared_ptr<Box> box) {
	if (!box->texture) return;

	// Clear the texture
	box->texture->clear();

	// Draw the bg texture
	sf::Sprite bg_sprite(box_bg);
	float scale = (float)BOX_RENDER_SIZE / (float)box_bg.getSize().x;
	bg_sprite.setScale(sf::Vector2f(scale, scale));
	box->texture->draw(bg_sprite);

	// If the player is in this box, render it
	if (player.container == box) {
		sf::Sprite player_sprite(player_tex);
		auto player_physical_position = player.body->GetPosition();
		auto child_render_pos = sf::Vector2f(player_physical_position.x * PIXELS_PER_METER, player_physical_position.y * PIXELS_PER_METER);
		player_sprite.setPosition(child_render_pos);
		player_sprite.setOrigin(sf::Vector2f(player_tex.getSize().x * .5f, player_tex.getSize().y * .5f));
		player_sprite.setScale(sf::Vector2f(.5f, .5f));
		box->texture->draw(player_sprite);
	}

	// Render and draw children
	for (auto child : box->children) {

		// Render the child
		render_box(child);

		// Draw the child texture to the slot
		if (child->texture) {

			// Create the render state
			sf::RenderStates child_states;

			// Calculate the child transform
			auto child_physical_pos = child->body->GetPosition();
			auto child_physical_ang = child->body->GetAngle();
			auto child_render_pos = sf::Vector2f(child_physical_pos.x * PIXELS_PER_METER, child_physical_pos.y * PIXELS_PER_METER);
			child_states.transform.translate(child_render_pos)
								  .rotate(child_physical_ang * 180.f / 3.14159f)
							      .scale(sf::Vector2f(1.f / (float)BOX_SLOTS, 1.f / (float)BOX_SLOTS));

			// Set up the child shader if necessary
			get_box_shader(child, child_states);

			// Draw the child texture
			sf::Sprite child_sprite(child->texture->getTexture());
			child_sprite.setOrigin(sf::Vector2f(BOX_PHYSICAL_SIZE * PIXELS_PER_METER * .5f, BOX_PHYSICAL_SIZE * PIXELS_PER_METER * .5f));
			box->texture->draw(child_sprite, child_states);
		}
	}

	// Draw blocks
	for (int sx = 0; sx < BOX_SLOTS; sx ++)
	for (int sy = 0; sy < BOX_SLOTS; sy++) {
		if (box->blocks[sx][sy] == 1) {
			sf::Sprite block_sprite(block_tex);
			sf::Rect<int> rect;
			rect.left = ceil(sx * block_tex.getSize().x / (float)BOX_SLOTS);
			rect.top = ceil(sy * block_tex.getSize().y / (float)BOX_SLOTS);
			rect.width = ceil(BOX_PIXELS_PER_SLOT);
			rect.height = ceil(BOX_PIXELS_PER_SLOT);
			block_sprite.setTextureRect(rect);
			block_sprite.setPosition(sf::Vector2f(sx * BOX_PIXELS_PER_SLOT, sy * BOX_PIXELS_PER_SLOT));
			box->texture->draw(block_sprite);
		}
	}

	//
	box->texture->display();
}

void game::get_box_shader(shared_ptr<Box> box, sf::RenderStates& render_states) {
	for (int face = 0; face < 4; face++) {
		auto door = box->doors[face];
		if (door && door->t > 0) {
			int face_pos;
			if (face == BoxFace::Top) face_pos = door->sx;
			else if (face == BoxFace::Right) face_pos = door->sy;
			else if (face == BoxFace::Bottom) face_pos = BOX_SLOTS - door->sx;
			else if (face == BoxFace::Left) face_pos = BOX_SLOTS - door->sy;

			open_meta_door_shader.setParameter("t", door->t);
			open_meta_door_shader.setParameter("face", face);
			open_meta_door_shader.setParameter("face_pos", face_pos);

			render_states.shader = &open_meta_door_shader;
			return;
		}
	}
}

shared_ptr<Box> game::add_box(shared_ptr<Box> parent, int sx, int sy) {

	// Create the box & add it to the box list
	auto box = shared_ptr<Box>(new Box());
	box->id = next_box_id ++;
	boxes.push_back(box);

	// Set the initial box state
	box->sx = box->target_sx = sx;
	box->sy = box->target_sy = sy;
	box->state = Gridded;

	// Generate a physics world for the new box
	box->world = shared_ptr<b2World>(new b2World(b2Vec2(0, GRAVITY)));
	generate_world_edges(box);

	// If this box is a child of another box...
	if (parent) {

		// Add the box to its parent's child list
		box->parent = parent;
		parent->children.push_back(box);

		// Create the new box's physics body in the parent's world
		float size = (float)BOX_PHYSICAL_SIZE / (float)BOX_SLOTS;
		b2BodyDef body_def;
		body_def.type = b2BodyType::b2_kinematicBody; //b2BodyType::b2_dynamicBody;
		body_def.userData = box.get();
		body_def.position = b2Vec2((sx + .5f) * size, (sy + .5f) * size);
		box->body = parent->world->CreateBody(&body_def);
		box->body->SetFixedRotation(true);

		// Add the main hull shape
		b2PolygonShape box_shape;
		box_shape.SetAsBox(size * .5f, size * .5f);
		auto fixture = box->body->CreateFixture(&box_shape, 1);
		fixture->SetFriction(FRICTION);
		b2Filter filter;
		filter.categoryBits = B2_CAT_BOX_HULL;
		filter.maskBits = B2_CAT_MAIN | B2_CAT_BOX_HULL;
		fixture->SetFilterData(filter);

		// Add the box's toggleable edges
		generate_box_edges(box);
	}

	// Give the box a texture space
	// TEMP
	assign_box_texture(box);

	//
	return box;
}

void game::add_block(shared_ptr<Box> parent, int sx, int sy) {

	// Set the box flag
	parent->blocks[sx][sy] = 1;

	// Create the physics
	float size = (float)BOX_PHYSICAL_SIZE / (float)BOX_SLOTS;
	b2BodyDef body_def;
	body_def.type = b2BodyType::b2_staticBody;
	body_def.position = b2Vec2((sx + .5f) * size, (sy + .5f) * size);
	b2Body* body = parent->world->CreateBody(&body_def);
	b2PolygonShape box_shape;
	box_shape.SetAsBox(size * .5f, size * .5f);
	auto fixture = body->CreateFixture(&box_shape, 1);
	fixture->SetFriction(FRICTION);
	b2Filter filter;
	filter.categoryBits = B2_CAT_MAIN;
	filter.maskBits = B2_CAT_MAIN;
	fixture->SetFilterData(filter);
}

void game::assign_box_texture(shared_ptr<Box> box) {
	shared_ptr<sf::RenderTexture> texture;
	if (unused_box_textures.size() > 0) {
		texture = *unused_box_textures.end();
		unused_box_textures.pop_back();
	}
	else {
		texture = shared_ptr<sf::RenderTexture>(new sf::RenderTexture());
		texture->create(BOX_RENDER_SIZE, BOX_RENDER_SIZE);
		box_textures.push_back(texture);
	}
	box->texture = texture;
}

void game::set_box_door(shared_ptr<Box> box, BoxFace face, int i, bool open) {
	int sx = 0;
	int sy = 0;
	if (face == Top) {
		sx = i;
	} else if (face == Right) {
		sx = BOX_SLOTS - 1;
		sy = i;
	} else if (face == Bottom) {
		sx = BOX_SLOTS - 1 - i;
		sy = BOX_SLOTS - 1;
	} else if (face == Left) {
		sy = BOX_SLOTS - 1 - i;
	}

	auto door = shared_ptr<BoxDoor>(new BoxDoor(box, false, 0, 0));
	door->sx = sx;
	door->sy = sy;
	box->doors[(int)face] = door;
	generate_box_edges(box);
	generate_world_edges(box);

	open_box_door(box, face, open);
}

void game::open_box_door(shared_ptr<Box> box, BoxFace face, bool open) {
	if (!box->doors[face]) return;
	if (open)
		for (int i = 0; i < 4; i++)
			if (box->doors[i])
				box->doors[i]->open = false;
	box->doors[face]->open = open;

	generate_box_edges(box);
	generate_world_edges(box);
	
}

void game::generate_box_edges(shared_ptr<Box> box) {
	if (!box->parent) return;
	if (!box->body) return;

	// Delete the existing edge fixture
	for (int i = 0; i < 4; i++) {
		if (box->body_edges[i]) {
			box->body->DestroyFixture(box->body_edges[i]);
			box->body_edges[i] = 0;
		}
	}

	// Generate new edge fixture
	b2Filter filter;
	filter.categoryBits = B2_CAT_MAIN;
	filter.maskBits = B2_CAT_MAIN;
	float half = (.5f * (float)BOX_PHYSICAL_SIZE / (float)BOX_SLOTS);
			   //+ (1.f / (float)PIXELS_PER_METER);
	for (int i_face = 0; i_face < 4; i_face++) {
		b2Vec2 a, b;
		b2EdgeShape edge_shape;
		b2Fixture* edge_fixture;
		shared_ptr<BoxDoor> door = box->doors[i_face];
		a.Set((i_face == 0 || i_face == 3 ? -1 : 1) * half, (i_face == 0 || i_face == 1 ? -1 : 1) * half);
		b.Set((i_face == 2 || i_face == 3 ? -1 : 1) * half, (i_face == 0 || i_face == 3 ? -1 : 1) * half);
		if (!(door && door->open)) {
			edge_shape.Set(a, b);
			edge_fixture = box->body->CreateFixture(&edge_shape, 0);
			edge_fixture->SetFriction(FRICTION);
			edge_fixture->SetFilterData(filter);
			box->body_edges[i_face] = edge_fixture;
		}
	}
}

void game::generate_world_edges(shared_ptr<Box> box) {
	if (!box->world) return;

	// Delete the existing edges body if there is one
	if (box->world_edges) {
		box->world->DestroyBody(box->world_edges);
		box->world_edges = NULL;
	}

	//
	b2BodyDef body_def;
	body_def.type = b2BodyType::b2_staticBody;
	box->world_edges = box->world->CreateBody(&body_def);
	b2Filter filter;
	filter.categoryBits = B2_CAT_MAIN;
	filter.maskBits = B2_CAT_MAIN | B2_CAT_BOX_HULL;
	float size = BOX_PHYSICAL_SIZE;

	//
	for (int i_face = 0; i_face < 4; i_face++) {
		b2Vec2 a, b;
		b2EdgeShape edge_shape;
		b2Fixture* edge_fixture;
		shared_ptr<BoxDoor> door = box->doors[i_face];
		a.Set((i_face == 0 || i_face == 3 ? 0 : size), (i_face == 0 || i_face == 1 ? 0 : size));
		b.Set((i_face == 2 || i_face == 3 ? 0 : size), (i_face == 0 || i_face == 3 ? 0 : size));
		if (door && door->open) {
			b2Vec2 a0, b0;
			if (i_face == Top) {
				a0.Set(door->sx, 0);
				b0.Set(door->sx + 1, 0);
			} else if (i_face == Right) {
				a0.Set(size, door->sy);
				b0.Set(size, door->sy + 1);
			} else if (i_face == Bottom) {
				a0.Set(door->sx + 1, size);
				b0.Set(door->sx, size);
			} else if (i_face == Left) {
				a0.Set(0, door->sy + 1);
				b0.Set(0, door->sy);
			}
			
			edge_shape.Set(a, a0);
			edge_fixture = box->world_edges->CreateFixture(&edge_shape, 0);
			edge_fixture->SetFriction(FRICTION);
			edge_fixture->SetFilterData(filter);

			edge_shape.Set(b0, b);
			edge_fixture = box->world_edges->CreateFixture(&edge_shape, 0);
			edge_fixture->SetFriction(FRICTION);
			edge_fixture->SetFilterData(filter);
		}
		else {
			edge_shape.Set(a, b);
			edge_fixture = box->world_edges->CreateFixture(&edge_shape, 0);
			edge_fixture->SetFriction(FRICTION);
			edge_fixture->SetFilterData(filter);
		}
	}
}

void game::set_player_container(shared_ptr<Box> box, b2Vec2 position) {
	set_player_container(box, position, b2Vec2(0, 0));
}
void game::set_player_container(shared_ptr<Box> box, b2Vec2 position, b2Vec2 velocity) {

	// If the player already has a body, delete it
	if (player.body) {
		player.body->GetWorld()->DestroyBody(player.body);
	}

	// Make a new body for the player in the new world
	float size = (float)BOX_PHYSICAL_SIZE / (float)BOX_SLOTS;
	b2BodyDef body_def;
	body_def.type = b2BodyType::b2_dynamicBody;
	body_def.userData = &player;
	body_def.position = position;
	body_def.linearVelocity = velocity;
	player.body = box->world->CreateBody(&body_def);
	player.body->SetFixedRotation(true);

	// Create a rectangular fixture for him
	b2PolygonShape player_shape;
	player_shape.SetAsBox(size * .3f, size * .3f);
	auto fixture = player.body->CreateFixture(&player_shape, 1);
	fixture->SetFriction(FRICTION);
	b2Filter filter;
	filter.categoryBits = B2_CAT_MAIN;
	filter.maskBits = B2_CAT_MAIN;
	fixture->SetFilterData(filter);

	// Set the view scale based on whether we are pushing or popping
	if (player.container) {

		// Popping
		if (player.container->parent == box) {
			view.scale = 2;
			center_view_on_slot(player.container->sx, player.container->sy, false);

		// Pushing
		} else if (box->parent == player.container) {
			view.scale = 1.f / (float)BOX_SLOTS;
		}
	}

	// Set the player container
	player.container = box;
}

void game::center_view_on_slot(int sx, int sy, bool target) {
	int x = (BOX_SLOTS * .5f - sx) * PIXELS_PER_METER * BOX_METERS_PER_SLOT;
	int y = (BOX_SLOTS * .5f - sy) * PIXELS_PER_METER * BOX_METERS_PER_SLOT;
	if (target) {
		view.tx = x;
		view.ty = y;
	} else {
		view.x = x;
		view.y = y;
	}
}