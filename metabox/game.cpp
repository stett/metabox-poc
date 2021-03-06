#include "game.h"
#include "settings.h"
#include "vec2f.h"
#include <SFML/OpenGL.hpp>
#include <SFML/System/Clock.hpp>

void game::setup() {
	next_box_id = 0;

	// Set up boxes
	auto a = add_box();
	root_box = a;
	set_box_door(a, Right, 5);
	auto b = add_box(a, 2, 5, true);

    auto c = add_box(a, 3, 4);
    set_box_door(c, Left, 0);
    set_box_door(c, Top, 3);

	auto d = add_box(a, 1, 4);
	set_box_door(d, Right, 6);
    set_box_door(d, Top, 3);

    /*auto e = add_box(c, 0, 6);
    set_box_door(e, Left, 0);

    auto f = add_box(d, 6, 6);
    set_box_door(f, Right, 6);*/

	for (int i = 0; i < 7; i++)
		add_block(a, i, 6);

	// Place the root box into a gravity-less root world
	outer_world = shared_ptr<b2World>(new b2World(b2Vec2(0, 0)));
	add_box_hull(root_box, outer_world, (float)BOX_PHYSICAL_SIZE / (float)BOX_SLOTS, 0, 0);
	//root_box->world = outer_world;

	// Add the player to the first box and give it a body
	set_player_container(a, b2Vec2(4, 3));

	auto* TEST = new sf::RenderWindow(
			sf::VideoMode(BOX_RENDER_SIZE, BOX_RENDER_SIZE),
			"Metabox Surfaces - Proof Of Concept",
			sf::Style::Close | sf::Style::Titlebar,
            sf::ContextSettings::ContextSettings(0, 0, 0, 3, 0));
	delete TEST;

	// Create the main window
	window = unique_ptr<sf::RenderWindow>(
		new sf::RenderWindow(
			sf::VideoMode(BOX_RENDER_SIZE, BOX_RENDER_SIZE),
			"Metabox Surfaces - Proof Of Concept",
			sf::Style::Close | sf::Style::Titlebar,
            sf::ContextSettings::ContextSettings(0, 0, 0, 3, 0)));
	window->setActive(true);

	// Create a graphical text to display
	font.loadFromFile("consola.ttf");

	// Load textures
	box_bg.loadFromFile("7grid.png");
	box_fg.loadFromFile("glass.png");
	grid.loadFromFile("7grid.png");
	player_tex.loadFromFile("player.png");
	block_tex.loadFromFile("block.png");

	// Load the open-meta-door shader
	meta_box_shader.loadFromFile("meta_box.vert", "meta_box.frag");
    meta_door_shader.loadFromFile("meta_door.vert", "meta_door.frag");

	//
	//set_mode(Edit);
}

void game::teardown() {
	window->close();
}

void game::run() {

	float dt_min = 1.f / 60.f;
	float dt_accum = 0;
	sf::Clock clock;
	sf::Time t0 = clock.getElapsedTime();
	sf::Time t1;

	while (mode != Quit) {
		
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
		for (auto box : boxes)
			if (box->world)
				box->world->ClearForces();
	};

	// Perform teardown actions before exiting program
	teardown();
}

void game::set_mode(Mode new_mode) {
	if (mode == new_mode) return;
	else mode = new_mode;

	if (mode == Play) {
		set_window_size(BOX_RENDER_SIZE, BOX_RENDER_SIZE);
	} else {
		sf::Vector2f pad(20, 60);
		auto desktop_mode = sf::VideoMode::getDesktopMode();
		set_window_size(desktop_mode.width - 2 * pad.x, desktop_mode.height - 2 * pad.y);
	}
}

void game::set_window_size(int width, int height) {
	auto desktop_mode = sf::VideoMode::getDesktopMode();
	window->setSize(sf::Vector2u(width, height));
	window->setView(sf::View(sf::FloatRect(0, 0, width, height)));
	window->setPosition(sf::Vector2i((desktop_mode.width - width) * .5f, (desktop_mode.height - height) * .5f));
}

void game::step(float dt) {

	// If the window has closed, stop the game
	if (!window->isOpen()) set_mode(Quit);

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

			if (event.key.code == sf::Keyboard::Up || event.key.code == sf::Keyboard::W) {
				player.body->ApplyForceToCenter(b2Vec2(0, -250), true);
			}

			/// TEMP ///
			if (event.key.code == sf::Keyboard::Q) {
				if (player.container->children.size()) {
					auto box = player.container->children.back();
					box->target_sx += 1;
				}
			}
			/// END TEMP ///
		}
	}

	// Apply forces to the player based on keyboard input
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) || sf::Keyboard::isKeyPressed(sf::Keyboard::D))
		player.body->ApplyForceToCenter(b2Vec2(12, 0), true);
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) || sf::Keyboard::isKeyPressed(sf::Keyboard::A))
		player.body->ApplyForceToCenter(b2Vec2(-12, 0), true);
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down) || sf::Keyboard::isKeyPressed(sf::Keyboard::S))
		player.body->ApplyForceToCenter(b2Vec2(0, 5), true);

	// Switch between program modes
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::F1)) set_mode(Play);
	else if (sf::Keyboard::isKeyPressed(sf::Keyboard::F2)) set_mode(Edit);
	else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)) set_mode(Quit);

	// Update all boxes
	for (auto box : boxes) {

		// Step the box's physics world
		if (box->world)
			box->world->Step(dt, 6, 2);

		// Update the box's phsyics body
		if (box->body) {
			auto pos = box->body->GetPosition();
			box->sx = (int)(pos.x * (float)BOX_SLOTS / (float)BOX_PHYSICAL_SIZE);
			box->sy = (int)(pos.y * (float)BOX_SLOTS / (float)BOX_PHYSICAL_SIZE);

			// If the box is gridded, move it towards its target slot
			if (box->state == Gridded) {
				auto target_pos = b2Vec2(
					(box->target_sx + .5f) * BOX_METERS_PER_SLOT,
					(box->target_sy + .5f) * BOX_METERS_PER_SLOT);
				auto diff = target_pos - pos;
				box->body->SetLinearVelocity(b2Vec2(diff.x * 5, diff.y * 5));
			}

			// Get pointer to new slot
			Slot* slot = 0;
			if (box->parent)
				slot = &box->parent->slots[box->sx][box->sy];

			// If the box's slot differs from its current slot position,
			// set the slot and recalculate adjacencies
			if (slot != box->slot) {

				// Remove from current slot
                auto old_slot = box->slot;
				if (old_slot)
                    old_slot->child = 0;

				// Add to new slot
                box->slot = slot;
				if (slot)
					slot->child = box;

                // Re-calculate box's door adjacencies
                find_door_adjacencies(box);
			}
		}

		// Update door transitions
		for (auto door : box->doors) {
			if (door) {
				if (door->open) {
					if (door->t < 1)
						door->t += 3 * dt;
					else door->t = 1;
				} else {
					if (door->t > 0)
						door->t -= 3 * dt;
					else door->t = 0;
				}
			}
		}
	}

	//
	if (nearest_door && nearest_door->open)
		open_box_door(nearest_door->box, nearest_door_face, false);

	//
	nearest_door = 0;
	float max_door_dist = 2;
	float nearest_door_dist = max_door_dist;

	// Close/open active metabox doors
	for (int face = 0; face < 4; face++) {
		auto door = player.container->doors[face];
		if (door) {

			// Get distance from player to door
			auto door_pos = sf::Vector2f(door->slot->x + .5f, door->slot->y + .5f) * BOX_METERS_PER_SLOT;
			auto player_pos = player.body->GetPosition();
			auto diff = vec2f(door_pos) - vec2f(player_pos);
			float dist = diff.length();
			
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
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) &&
		nearest_door &&
		nearest_door_dist < max_door_dist)
		open_box_door(nearest_door->box, nearest_door_face, true);

	// Process player meta-transitions
	// TODO: This *should* transfer to the ADJACENT box, not just always to the parent.
	//       Once the adjacency system is up we can do it that way. Doing it the right
	//		 way will prevent the need to separate entering/exiting metaboxes, and will
	//		 also automatically facilitate "lateral" transitions in the box-tree. Woot.
	{
		bool player_transfered = false;

		// If the player has wandered out of a metadoor,
		// transfer them to the parent box.
		auto player_pos = player.body->GetPosition();
		float max_pos = (float)BOX_PHYSICAL_SIZE;
		if (player_pos.x < 0 || player_pos.y < 0 || player_pos.x > max_pos || player_pos.y > max_pos) {

			// If the player is leaving the current top recursive meta, then set that as the container.
            // Otherwise, leave it as the current player.container.
			auto container = player.container;
			if (!player.recursions.empty() && player.recursions.top()->parent == player.container) {
                nearest_door->t = 0;
				container = player.recursions.top();
			}

			// Calculate the new player position
			float off = .5f * (float)BOX_PHYSICAL_SIZE / (float)BOX_SLOTS;
			player_pos -= b2Vec2(.5f * BOX_PHYSICAL_SIZE, .5f * BOX_PHYSICAL_SIZE);
			player_pos.x /= (float)BOX_SLOTS;
			player_pos.y /= (float)BOX_SLOTS;
			player_pos += container->body->GetPosition();

			// Set the new player container
			set_player_container(container->parent, player_pos, player.body->GetLinearVelocity());
			player_transfered = true;
		}

		// If the player has wandered into a sub-meta door,
		// transfer them into the child box.
		for (auto child : player.container->children) {

            // If there is one, find the open door for this child.
			shared_ptr<BoxDoor> door = 0;
			for (int i = 0; i < 4; i++) {
				if (child->doors[i] && child->doors[i]->open) {
					door = child->doors[i];
					break;
				}
			}
			if (!door) continue;

            // If we already found a transition, stop checking
			if (player_transfered) break;

            // Check this child for overlap.
			for (b2Fixture* fixture = child->body->GetFixtureList(); fixture; fixture = fixture->GetNext()) {
				if (fixture->GetType() != b2Shape::Type::e_polygon) continue;
				if (fixture->GetShape()->TestPoint(child->body->GetTransform(), player.body->GetPosition())) {
					player_pos -= child->body->GetPosition();
					player_pos += b2Vec2(
                        door->slot->x * BOX_PHYSICAL_SIZE / BOX_SLOTS,
                        door->slot->y * BOX_PHYSICAL_SIZE / BOX_SLOTS);
					player_pos += b2Vec2(.5f * BOX_PHYSICAL_SIZE / BOX_SLOTS, .5f * BOX_PHYSICAL_SIZE / BOX_SLOTS);

                    // If we're transfering to a recursive submeta, open the superdoor
                    if (child->recursive)
                        child->parent->doors[door->face]->t = door->t;

                    // Transfer to this child
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
}


void game::find_door_adjacencies(shared_ptr<Box> box) {

    // Perform adjacency calculation on all doors
    for (auto door : box->doors)
        if (door) find_door_adjacency(door);
}

void game::find_door_adjacency(shared_ptr<BoxDoor> door) {
    //
    // Looks for a door adjacent to this one
    //

    if (!door) return;

    // Contract herpes
    find_door_shingle(door);

    // Rub up against a slot
    BoxFace face = door->face;
    Slot* slot = door->box->slot;
    Slot* adj_slot = get_adjacent_slot(slot, door->face);

    // If the adjacent slot exists and is "great with child",
    // then there might be an adjacent door.
    shared_ptr<BoxDoor> adjacency = 0;
    if (adj_slot && adj_slot->child) {
        auto adj_box = adj_slot->child;
        auto opposing_door = adj_box->doors[(face + 2) % 4];

        // If the opposing door lines up with this one, then we have an adjacency!
        if (((face == Left || face == Right) && door->slot->y == opposing_door->slot->y) ||
            ((face == Top || face == Bottom) && door->slot->x == opposing_door->slot->x)) {
            adjacency = opposing_door;
        }
    }

    // If the door already has an adjacency and it isn't equal to this one, recalculate it
    if (door->adjacency && door->adjacency != adjacency)
        find_door_adjacency(door->adjacency);

    // Symetrically set the adjacencies
    door->adjacency = adjacency;
    if (adjacency)
        adjacency->adjacency = door;

    // If the slots containing this door or it's newly set adjacent door either have a children,
    // their sub-adjacencies may need updating
    if (door->slot->child) find_door_adjacency(
        door->slot->child->doors[door->face]);
    if (adjacency && adjacency->slot->child) find_door_adjacency(
        adjacency->slot->child->doors[door->face]);
}

void game::find_door_shingle(shared_ptr<BoxDoor> door) {
    //
    // Finds the higher shingle for a door.
    //

    shared_ptr<BoxDoor> shingle = 0;

    // If the door's box doesn't have a parent, then there's no possibility of shingling.
    // Also, if the door's box's slot isn't on an edge, then it can't shingle.
    auto parent = door->box->parent;
    auto slot = door->box->slot;
    if (parent && slot->edges(door->face)) {

        // If the door is "shingled" by another door, save the shingling.
        // We start by setting it to the door in the parent's appropriate face,
        // and nulling it if it doesn't line up properly.
        shingle = parent->doors[door->face];
        if (shingle) {
            if (((door->face == Left || door->face == Right) && shingle->slot->y != slot->y) ||
                ((door->face == Top || door->face == Bottom) && shingle->slot->x != slot->x))
                shingle = 0;
        }
    }

    // If the shingle hasn't changed, stop here
    if (door->shingle_up == shingle)
        return;

    // Cancel the old shingle's shingle-down
    if (door->shingle_up)
        door->shingle_up->shingle_down = 0;

    // Set the shingle door
    door->shingle_up = shingle;
    if (shingle)
        shingle->shingle_down = door;
}

Slot* game::get_adjacent_slot(Slot* slot, BoxFace face) {
    if (!slot) return 0;

    Slot* adj_slot = 0;

    if (slot->edges(face)) {
        auto door = slot->parent->doors[face];
        if (door->adjacency && (
            ((face == Left || face == Right) && door->slot->y == slot->y) ||
            ((face == Top || face == Bottom) && door->slot->x == slot->x))) {
            adj_slot = door->adjacency->slot;
        }
    } else {
        if (face == Left)        adj_slot = &slot->parent->slots[slot->x - 1][slot->y];
        else if (face == Right)  adj_slot = &slot->parent->slots[slot->x + 1][slot->y];
        else if (face == Top)    adj_slot = &slot->parent->slots[slot->x][slot->y - 1];
        else if (face == Bottom) adj_slot = &slot->parent->slots[slot->x][slot->y + 1];
    }

    return adj_slot;
}


void game::draw() {

	// Clear screen
	window->clear(sf::Color(100, 100, 100));

	//
	if (mode == Play) render_game();
	else if (mode == Edit) render_editor();

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
	auto active_child = active_box;
	auto active_parent = active_box->parent;

	// If we are in a recursive submeta, set the active parent as the active box itself,
	// and set the active child as the recursive sub meta. Jeez.
	bool recursive_parent = !player.recursions.empty() && player.recursions.top()->parent == active_box;
	if (recursive_parent) {
		active_parent = active_box;
		active_child = player.recursions.top();
	}

	// If the active box has a parent
	if (active_parent) {
		render_box(active_parent);
		sf::Sprite sprite(active_parent->texture->getTexture());
		sf::RenderStates states;

		// Apply view transformations
		get_view_transforms(states);

		// Apply shaders to the parent box
		get_box_shader(active_parent, states, !recursive_parent);

		// Up-scale and position the parent box
		vec2f pos = vec2f(active_child->sx, active_child->sy) * PIXELS_PER_METER * BOX_METERS_PER_SLOT;
		states.transform.scale(sf::Vector2f(BOX_SLOTS, BOX_SLOTS))
			  .translate(-pos.toVector2f());

		// Draw the parent
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

	// Draw the foreground texture with alpha inversely-
	// proportional to the zoom level
	if (view.scale < 1) {
		states.shader = 0;
		sf::Sprite fg_sprite(*active_box->fg);
		fg_sprite.setColor(sf::Color(255, 255, 255, 255.f * (1 - view.scale)));
		fg_sprite.setScale(sf::Vector2f(
			(float)BOX_RENDER_SIZE / (float)active_box->fg->getSize().x,
			(float)BOX_RENDER_SIZE / (float)active_box->fg->getSize().y));
		window->draw(fg_sprite, states);
	}
}

void game::render_editor() {

	// Render the boxes to their textures
	render_box(root_box);

	// Decide on a box size and max number of boxes per row
	float box_pad = 26.0f;
	float box_size = window->getSize().x / 3.0f - 4.0f * box_pad;
	float box_scale = box_size / (float)BOX_RENDER_SIZE;
	int boxes_per_row = 3;

	// Draw all the box textures & calculate thier positions
	sf::Vector2i ipos(0, 0);
	map<int, sf::Vector2f> box_positions;
	for (auto box : boxes) {
		if (!box->texture) continue;

		// Convert the rendered box texture to a sprite and draw it to the screen
		sf::Sprite sprite(box->texture->getTexture());
		sprite.setScale(sf::Vector2f(box_scale, box_scale));
		sf::Vector2f pos(box_pad + ipos.x * (box_size + box_pad), box_pad + ipos.y * (box_size + box_pad));
		box_positions[box->id] = pos;
		sprite.setPosition(pos);
		sf::RenderStates states;
		get_box_shader(box, states);
		window->draw(sprite, states);

		// Draw the box's fg texture
		if (box != player.container) {
			states.shader = 0;
			sf::Sprite fg_sprite(*box->fg);
			fg_sprite.setPosition(pos);
			fg_sprite.setScale(sf::Vector2f(
				box_size / (float)box->fg->getSize().x,
				box_size / (float)box->fg->getSize().y));
			window->draw(fg_sprite, states);
		}

		// Calculate the next position indices
		ipos.x++;
		if (ipos.x >= boxes_per_row) {
			ipos.x = 0;
			ipos.y++;
		}
	}

	// Render box overlay data
	for (auto box : boxes) {
		if (!box->texture) continue;

		// Get the position of this box on the gui
		auto box_position = box_positions[box->id];

		// Highlight the active box
		if (box == player.container) {

			// TODO
		}

		// Draw grid
		for (int i = 0; i < BOX_SLOTS; i++) {
			sf::Transform transform;
			transform.translate(box_position);
			sf::Vertex h_line[2];
			sf::Vertex v_line[2];
			v_line[0].position.x = h_line[0].position.y =
			v_line[1].position.x = h_line[1].position.y = i * box_size / (float)BOX_PHYSICAL_SIZE;
			v_line[0].position.y = h_line[0].position.x = 0;
			v_line[1].position.y = h_line[1].position.x = box_size;
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
			transform
				.translate(box_position)
				.scale(sf::Vector2f(box_scale, box_scale))
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
            auto door = box->doors[i];
            if (!door) continue;

			sf::Transform transform;
			transform
				.translate(box_position)
				.scale(sf::Vector2f(box_scale, box_scale));

			sf::Vector2f pos(
                door->slot->x * BOX_METERS_PER_SLOT * PIXELS_PER_METER,
                door->slot->y * BOX_METERS_PER_SLOT * PIXELS_PER_METER);

			sf::RectangleShape rect;
			rect.setPosition(pos);
			rect.setSize(sf::Vector2f(BOX_METERS_PER_SLOT * PIXELS_PER_METER, BOX_METERS_PER_SLOT * PIXELS_PER_METER));

            if (door->open) {
				rect.setOutlineColor(sf::Color::Blue);
				rect.setFillColor(sf::Color(0, 0, 255, 50));
			} else {
				rect.setOutlineColor(sf::Color::Red);
				rect.setFillColor(sf::Color(255, 0, 0, 50));
			}

			window->draw(rect, sf::RenderStates(transform));

            // Draw line to adjacent door if there is one
            if (door->adjacency) {
                auto other_body_pos = box_positions[door->adjacency->box->id];

                sf::Vertex line[2];
                auto body_pos = box->body->GetPosition();
                line[0].position = box_position + sf::Vector2f(pos.x * box_scale, pos.y * box_scale);
                line[1].position = box_positions[door->adjacency->box->id] + sf::Vector2f(
                    door->adjacency->slot->x * BOX_METERS_PER_SLOT * PIXELS_PER_METER * box_scale,
                    door->adjacency->slot->y * BOX_METERS_PER_SLOT * PIXELS_PER_METER * box_scale);
                window->draw(line, 2, sf::PrimitiveType::Lines);
            }
		}

		// Child data
		for (auto child : box->children) {
			sf::Transform transform;
			transform.translate(box_position);

			float size = box_size / (float)BOX_SLOTS;

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
			text.setPosition(sf::Vector2f(child->sx, child->sy) * size + sf::Vector2f(3, 3));
			text.setColor(sf::Color(0, 0, 0, 255));
			window->draw(text, sf::RenderStates(transform));
			text.setPosition(sf::Vector2f(child->sx, child->sy) * size + sf::Vector2f(2, 2));
			text.setColor(sf::Color(255, 255, 255, 255));
			window->draw(text, sf::RenderStates(transform));
		}

		// Parent/child arrows
		if (box->parent) {
			sf::Vertex line[2];
			auto body_pos = box->body->GetPosition();
			line[0].position = box_position;
			line[1].position = box_positions[box->parent->id] + sf::Vector2f(body_pos.x * PIXELS_PER_METER * box_scale, body_pos.y * PIXELS_PER_METER * box_scale);
			window->draw(line, 2, sf::PrimitiveType::Lines);
		}

		// Draw the box identifier string
		sf::Text text(to_string(box->id), font, 14);
		text.setPosition(box_position + sf::Vector2f(3, 3));
		text.setColor(sf::Color(0, 0, 0, 255));
		window->draw(text);
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
	{
		sf::Sprite bg_sprite(*box->bg);
		float scale = (float)BOX_RENDER_SIZE / (float)box->bg->getSize().x;
		bg_sprite.setScale(sf::Vector2f(
			(float)BOX_RENDER_SIZE / (float)box->bg->getSize().x,
			(float)BOX_RENDER_SIZE / (float)box->bg->getSize().y));
		box->texture->draw(bg_sprite);
	}

	// If the player is in this box, render him
	if (player.container == box) {
		sf::Sprite player_sprite(player_tex);
		auto player_physical_position = player.body->GetPosition();
		auto child_render_pos = sf::Vector2f(player_physical_position.x * PIXELS_PER_METER, player_physical_position.y * PIXELS_PER_METER);
		player_sprite.setPosition(child_render_pos);
		player_sprite.setOrigin(sf::Vector2f(player_tex.getSize().x * .5f, player_tex.getSize().y * .5f));
		player_sprite.setScale(sf::Vector2f(.5f, .5f));
		box->texture->draw(player_sprite);
	}

	// Render and draw non-recursive children
	for (auto child : box->children)
		if (!child->recursive)
			render_child(box, child);

	// Draw blocks
	for (int sx = 0; sx < BOX_SLOTS; sx++)
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

	// Draw walls
	float thickness = 6.f;
	for (int face = 0; face < 4; face++) {
		sf::Sprite block_sprite(block_tex);

        // Stretch out a block-texture. This is not ideal.
		if ((BoxFace)face == Top || (BoxFace)face == Bottom) {
			block_sprite.setScale(sf::Vector2f(
				(float)BOX_SLOTS * (float)BOX_PIXELS_PER_SLOT / block_tex.getSize().x,
				thickness / block_tex.getSize().y));
		} else {
			block_sprite.setScale(sf::Vector2f(
				thickness / block_tex.getSize().x,
				(float)BOX_SLOTS * (float)BOX_PIXELS_PER_SLOT / block_tex.getSize().y));
		}

        // Shift the block texture if necessary.
		if ((BoxFace)face == Right) {
			block_sprite.setPosition(sf::Vector2f(BOX_SLOTS * BOX_PIXELS_PER_SLOT - thickness, 0));
		} else if ((BoxFace)face == Bottom) {
			block_sprite.setPosition(sf::Vector2f(0, BOX_SLOTS * BOX_PIXELS_PER_SLOT - thickness));
		}

        // Add open/closed door shader to render states if there's a door in this wall
        sf::RenderStates states;
        auto door = box->doors[face];
        if (door) {

            static sf::Clock clock;
            sf::Time t = clock.getElapsedTime();

            int face_pos;
            if (face == BoxFace::Top) face_pos = door->slot->x;
            else if (face == BoxFace::Right) face_pos = door->slot->y;
            else if (face == BoxFace::Bottom) face_pos = BOX_SLOTS - door->slot->x;
            else if (face == BoxFace::Left) face_pos = BOX_SLOTS - door->slot->y;

            meta_door_shader.setParameter("t", (door->adjacency ? 1.0f : door->t));
            meta_door_shader.setParameter("face", face);
            meta_door_shader.setParameter("face_pos", face_pos);
            meta_door_shader.setParameter("seed", t.asSeconds());
            states.shader = &meta_door_shader;
        }

		box->texture->draw(block_sprite, states);
	}

	// Draw all recursive children
	for (auto child : box->children)
		if (child->recursive)
			render_child(box, child);

	//
	box->texture->display();
}

void game::render_child(shared_ptr<Box> parent, shared_ptr<Box> child) {

	// Render the child
	//if (!child->recursive)
	//	render_box(child);

	// Get the child's texture
	shared_ptr<sf::RenderTexture> child_texture = 0;
	if (child->recursive) {
		child_texture = parent->texture;
	} else {
		render_box(child);
		child_texture = child->texture;
	}

	// Draw the child texture to the slot
	if (child_texture) {

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
		sf::Sprite child_sprite(child_texture->getTexture());
		child_sprite.setOrigin(sf::Vector2f(BOX_PHYSICAL_SIZE * PIXELS_PER_METER * .5f, BOX_PHYSICAL_SIZE * PIXELS_PER_METER * .5f));
		parent->texture->draw(child_sprite, child_states);

		// Draw the child's fg texture
		child_states.shader = 0;
		sf::Sprite fg_sprite(*parent->fg);
		fg_sprite.setOrigin((vec2f(parent->fg->getSize()) * .5f).toVector2f());
		fg_sprite.setScale(sf::Vector2f(
			(float)BOX_RENDER_SIZE / (float)parent->fg->getSize().x,
			(float)BOX_RENDER_SIZE / (float)parent->fg->getSize().y));
		parent->texture->draw(fg_sprite, child_states);
	}
}

void game::get_box_shader(shared_ptr<Box> box, sf::RenderStates& render_states, bool door_shader, bool entropy_shader) {

	// Make a static clock to use for random seeding later
	static sf::Clock clock;
	sf::Time t = clock.getElapsedTime();

	// Reset the door transition time variable and the entropy variable based on recursion depth
	meta_box_shader.setParameter("t", 0);
	float entropy = player.recursions.size();
	if (entropy_shader) {
		meta_box_shader.setParameter("entropy", entropy);
		meta_box_shader.setParameter("seed", t.asSeconds());
	}

	// Set the door transition
	if (door_shader) {
		for (int face = 0; face < 4; face++) {
			auto door = box->doors[face];
			if (door && door->t > 0) {
				int face_pos;
				if (face == BoxFace::Top) face_pos = door->slot->x;
				else if (face == BoxFace::Right) face_pos = door->slot->y;
				else if (face == BoxFace::Bottom) face_pos = BOX_SLOTS - door->slot->x;
				else if (face == BoxFace::Left) face_pos = BOX_SLOTS - door->slot->y;

				meta_box_shader.setParameter("t", door->t);
				meta_box_shader.setParameter("face", face);
				meta_box_shader.setParameter("face_pos", face_pos);

				break;
			}
		}
	}

	// Set the shader pointer
	if (entropy_shader || door_shader)
		render_states.shader = &meta_box_shader;
}

shared_ptr<Box> game::add_box(shared_ptr<Box> parent, int sx, int sy, bool recursive) {

	// Create the box & add it to the box list
	auto box = shared_ptr<Box>(new Box());
	box->id = next_box_id ++;
	boxes.push_back(box);

	// Set the initial box state
	box->sx = box->target_sx = sx;
	box->sy = box->target_sy = sy;
	box->state = Gridded;

	// Set recursiveness
	box->recursive = recursive;

	// If it's not recursive, do stuff for normal boxes that doesn't apply to recursive boxes.
	if (!recursive) {

		// Set bg and fg textures
		box->bg = &box_bg;
		box->fg = &box_fg;

		// Generate a physics world for the new box
		box->world = shared_ptr<b2World>(new b2World(b2Vec2(0, GRAVITY)));
		generate_world_edges(box);

		// Give the box a texture space
		// TEMP
		assign_box_texture(box);
	}

	// If it's recursive, set the same doors on it as it's parent
	if (recursive && parent) {
		for (int i = 0; i < 4; i++) {
			auto door = parent->doors[i];
			if (door) {
				set_box_door(box, (BoxFace)i, door->slot, door->open);
			}
		}
	}

	// If this box is a child of another box...
	if (parent) {

		// Add the box to its parent's child list
		box->parent = parent;
		parent->children.push_back(box);

		// Add
		add_box_hull(box, parent->world, (float)BOX_PHYSICAL_SIZE / (float)BOX_SLOTS, sx, sy);
	}

	//
	return box;
}

void game::add_box_hull(shared_ptr<Box> box, shared_ptr<b2World> world, float size, int sx, int sy) {

	// Create the new box's physics body in the parent's world
	b2BodyDef body_def;
	body_def.type = b2BodyType::b2_kinematicBody; //b2BodyType::b2_dynamicBody;
	body_def.userData = box.get();
	body_def.position = b2Vec2((sx + .5f) * size, (sy + .5f) * size);
	box->body = world->CreateBody(&body_def);
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

	set_box_door(box, face, &box->slots[sx][sy], open);
}

void game::set_box_door(shared_ptr<Box> box, BoxFace face, Slot* slot, bool open) {
	auto door = shared_ptr<BoxDoor>(new BoxDoor(box, face, false, slot));
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
	
	// Open same door for recursive children
	for (auto child : box->children)
		if (child->recursive)
			open_box_door(child, face, open);
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
				a0.Set(door->slot->x, 0);
				b0.Set(door->slot->x + 1, 0);
			} else if (i_face == Right) {
				a0.Set(size, door->slot->y);
				b0.Set(size, door->slot->y + 1);
			} else if (i_face == Bottom) {
				a0.Set(door->slot->x + 1, size);
				b0.Set(door->slot->x, size);
			} else if (i_face == Left) {
				a0.Set(0, door->slot->y + 1);
				b0.Set(0, door->slot->y);
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

	// If we're transfering to "no-box", set the world as the outer-world
	//auto world = (box ? (box->recursive ? box->parent->world : box->world) : outer_world);

    // Generate the new player body
    //player.generate_body(world, position, velocity);

    

	// Set the view scale based on whether we are pushing or popping
	if (player.container) {

		// Popping
		if (player.container->parent == box) {
			view.scale = (float)BOX_SLOTS * .5f;
			center_view_on_slot(player.container->sx, player.container->sy, false);

		// Popping recursively
		} else if (player.container == box) {
			view.scale = (float)BOX_SLOTS * .5f;
			auto container = player.recursions.top();
			center_view_on_slot(container->sx, container->sy, false);
			player.recursions.pop();

		// Popping into outer-world
		//} else if (!player.container->parent) {
			//center_view_on_player();

		// Pushing
		} else if (box->parent == player.container) {
			view.scale = 1.f / (float)BOX_SLOTS;
			center_view_on_parent_slot(box->sx, box->sy, false);

			// If we're pushing into a recursive box, add it to the recursive stack
			if (box->recursive) {
				player.recursions.push(box);
			}
		}
	}

	// Set the player container
	//player.container = (box ? (box->recursive ? box->parent : box) : 0);
    player.set_container(box ? (box->recursive ? box->parent : box) : 0, position, velocity);
}

void game::center_view_on_slot(int sx, int sy, bool target) {
	int x = ((BOX_SLOTS - 1) * .5f - sx) * PIXELS_PER_METER * BOX_METERS_PER_SLOT;
	int y = ((BOX_SLOTS - 1) * .5f - sy) * PIXELS_PER_METER * BOX_METERS_PER_SLOT;
	if (target) {
		view.tx = x;
		view.ty = y;
	} else {
		view.x = x;
		view.y = y;
	}
}

void game::center_view_on_parent_slot(int sx, int sy, bool target) {
	int x = -PIXELS_PER_METER * BOX_METERS_PER_SLOT * ((BOX_SLOTS - 1) * .5f - sx) / view.scale;
	int y = -PIXELS_PER_METER * BOX_METERS_PER_SLOT * ((BOX_SLOTS - 1) * .5f - sy) / view.scale;
	if (target) {
		view.tx = x;
		view.ty = y;
	}
	else {
		view.x = x;
		view.y = y;
	}
}