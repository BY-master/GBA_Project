#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_memory.h"
#include "bn_bg_tiles.h"
#include "bn_bg_palettes.h"
#include "bn_affine_bg_ptr.h"
#include "bn_affine_bg_item.h"
#include "bn_affine_bg_map_ptr.h"
#include "bn_affine_bg_map_cell_info.h"
#include "bn_regular_bg_ptr.h"
#include "bn_regular_bg_item.h"
#include "bn_regular_bg_map_ptr.h"
#include "bn_regular_bg_map_cell_info.h"
#include "bn_camera_actions.h"
#include "bn_music_actions.h"
#include "bn_sprite_text_generator.h"
#include "bn_sprites_mosaic_actions.h"
#include "bn_bgs_mosaic_actions.h"
#include "bn_mosaic_attributes.h"
#include "bn_mosaic_attributes_hbe_ptr.h"
// graphics
#include "bn_sprite_items_cursor.h"
#include "bn_sprite_items_dog.h"
#include "bn_sprite_items_blonde.h"
#include "bn_affine_bg_items_red.h"
#include "bn_affine_bg_tiles_items_tiles.h"
#include "bn_regular_bg_items_land.h"
#include "bn_regular_bg_items_land_2.h"
#include "bn_regular_bg_tiles_items_tiles_2.h"
#include "bn_regular_bg_items_map.h"
#include "bn_bg_palette_items_palette.h"
#include "bn_bg_palette_items_palette_2.h"
// audio
#include "bn_music_items.h"
// common
#include "common_info.h"
#include "common_variable_8x16_sprite_font.h"

namespace
{
    void bg_visiblity_scene(bn::sprite_text_generator& text_generator)
    {
        constexpr bn::string_view info_text_lines[] = {
            "GBA Game!"
        };

        common::info info("BGs visiblity", info_text_lines, text_generator);

        bn::affine_bg_ptr red_bg = bn::affine_bg_items::red.create_bg(0, 0);

        while(!bn::keypad::start_pressed())
        {
            if(bn::keypad::a_pressed())
            {
                red_bg.set_visible(!red_bg.visible());
            }

            info.update();
            bn::core::update();
        }
    }

    void music_scene(bn::sprite_text_generator& text_generator)
    {
        constexpr bn::string_view info_text_lines[] = {
            "Music!"
        };

        common::info info("Music", info_text_lines, text_generator);

        bn::music_items::cyberrid.play(0.5);

        while(!bn::keypad::start_pressed())
        {
            if(bn::keypad::a_pressed())
            {
                if(bn::music::paused())
                {
                    bn::music::resume();
                }
                else
                {
                    bn::music::pause();
                }
            }

            info.update();
            bn::core::update();
        }
        
        bn::music::stop();
    }

    void land_scene()
    {
        bn::regular_bg_ptr land_bg = bn::regular_bg_items::land.create_bg(0, 0);
        bn::camera_ptr camera = bn::camera_ptr::create(0, 0);
        land_bg.set_camera(camera);

        while(!bn::keypad::start_pressed())
        {
            if(bn::keypad::left_held())
            {
                camera.set_x(camera.x() - 1);
            }
            else if(bn::keypad::right_held())
            {
                camera.set_x(camera.x() + 1);
            }

            if(bn::keypad::up_held())
            {
                camera.set_y(camera.y() - 1);
            }
            else if(bn::keypad::down_held())
            {
                camera.set_y(camera.y() + 1);
            }

            bn::core::update();
        }
    }

    struct bg_map
    {
        static const int columns = 32;
        static const int rows = 32;

        alignas(int) bn::affine_bg_map_cell cells[columns * rows];
        bn::affine_bg_map_item map_item;

        bg_map() :
            map_item(cells[0], bn::size(bg_map::columns, bg_map::rows))
        {
            reset();
        }

        bool dig(int x, int y)
        {
            bn::affine_bg_map_cell& current_cell = cells[map_item.cell_index(x, y)];
            bn::affine_bg_map_cell_info cell_info(current_cell);
            // tile_index == 1 已挖过
            if(cell_info.tile_index() == 1)
            {
                return false;
            }
            // mark
            cell_info.set_tile_index(1);
            current_cell = cell_info.cell();

            bn::affine_bg_map_cell& up_cell = cells[map_item.cell_index(x, y - 1)];
            cell_info.set_cell(up_cell);

            if(cell_info.tile_index() == 0)
            {
                cell_info.set_tile_index(2);
                up_cell = cell_info.cell();
            }

            return true;
        }

        void reset()
        {
            bn::memory::clear(cells);
        }
    };

    bn::fixed sprite_x(int cursor_x)
    {
        return (cursor_x * 8) - (bg_map::columns * 4) + 4;
    }

    bn::fixed sprite_y(int cursor_y)
    {
        return (cursor_y * 8) - (bg_map::rows * 4) + 4;
    }

    constexpr int ground_tile_index = 1;

    constexpr int wall_top_cornor_tile_index = 2;
    constexpr int wall_top_middle_tile_index = 3;
    constexpr int wall_middle_tile_index = 4;
    constexpr int wall_bottom_cornor_tile_index = 5;
    constexpr int wall_bottom_middle_tile_index = 6;

    struct regular_bg_map
    {
        static constexpr int columns = 32;
        static constexpr int rows = 32;

        alignas(int) bn::regular_bg_map_cell cells[columns * rows];
        bn::regular_bg_map_item map_item;

        regular_bg_map():
            map_item(cells[0], bn::size(regular_bg_map::columns, regular_bg_map::rows))
        {
            reset();
        }

        void dig(int x, int y)
        {
            bn::regular_bg_map_cell& current_cell = cells[map_item.cell_index(x, y)];
            bn::regular_bg_map_cell_info current_cell_info(current_cell);
            current_cell_info.set_tile_index(ground_tile_index);
            current_cell_info.set_palette_id(1);
            current_cell_info.set_horizontal_flip(false);
            current_cell = current_cell_info.cell();

            _update_wall(x - 1, y - 1);
            _update_wall(x, y - 1);
            _update_wall(x + 1, y - 1);

            _update_wall(x - 1, y);
            _update_wall(x + 1, y);

            _update_wall(x - 1, y + 1);
            _update_wall(x, y + 1);
            _update_wall(x + 1, y + 1);
        }

        void reset()
        {
            bn::memory::clear(cells);
        }

    private:
        void _update_wall(int x, int y)
        {
            bn::regular_bg_map_cell& current_cell = cells[map_item.cell_index(x, y)];
            bn::regular_bg_map_cell_info current_cell_info(current_cell);

            if(current_cell_info.tile_index() == ground_tile_index)
            {
                current_cell_info.set_palette_id(0);
                current_cell = current_cell_info.cell();
                return;
            }

            bn::regular_bg_map_cell_info down_cell_info(cells[map_item.cell_index(x, y + 1)]);

            if(down_cell_info.tile_index() == ground_tile_index)
            {
                current_cell_info.set_tile_index(wall_top_middle_tile_index);
                current_cell_info.set_palette_id(0);
                current_cell_info.set_horizontal_flip(false);
                current_cell = current_cell_info.cell();
                return;
            }

            bn::regular_bg_map_cell_info right_cell_info(cells[map_item.cell_index(x + 1, y)]);

            if(right_cell_info.tile_index() == ground_tile_index)
            {
                current_cell_info.set_tile_index(wall_middle_tile_index);
                current_cell_info.set_palette_id(0);
                current_cell_info.set_horizontal_flip(false);
                current_cell = current_cell_info.cell();
                return;
            }

            bn::regular_bg_map_cell_info left_cell_info(cells[map_item.cell_index(x - 1, y)]);

            if(left_cell_info.tile_index() == ground_tile_index)
            {
                current_cell_info.set_tile_index(wall_middle_tile_index);
                current_cell_info.set_palette_id(0);
                current_cell_info.set_horizontal_flip(true);
                current_cell = current_cell_info.cell();
                return;
            }

            bn::regular_bg_map_cell_info up_cell_info(cells[map_item.cell_index(x, y - 1)]);

            if(up_cell_info.tile_index() == ground_tile_index)
            {
                current_cell_info.set_tile_index(wall_bottom_middle_tile_index);
                current_cell_info.set_palette_id(0);
                current_cell_info.set_horizontal_flip(false);
                current_cell = current_cell_info.cell();
                return;
            }

            bn::regular_bg_map_cell_info right_down_cell_info(cells[map_item.cell_index(x + 1, y + 1)]);

            if(right_down_cell_info.tile_index() == ground_tile_index)
            {
                current_cell_info.set_tile_index(wall_top_cornor_tile_index);
                current_cell_info.set_palette_id(0);
                current_cell_info.set_horizontal_flip(false);
                current_cell = current_cell_info.cell();
                return;
            }

            bn::regular_bg_map_cell_info left_down_cell_info(cells[map_item.cell_index(x - 1, y + 1)]);

            if(left_down_cell_info.tile_index() == ground_tile_index)
            {
                current_cell_info.set_tile_index(wall_top_cornor_tile_index);
                current_cell_info.set_palette_id(0);
                current_cell_info.set_horizontal_flip(true);
                current_cell = current_cell_info.cell();
                return;
            }

            bn::regular_bg_map_cell_info right_up_cell_info(cells[map_item.cell_index(x + 1, y - 1)]);

            if(right_up_cell_info.tile_index() == ground_tile_index)
            {
                current_cell_info.set_tile_index(wall_bottom_cornor_tile_index);
                current_cell_info.set_palette_id(0);
                current_cell_info.set_horizontal_flip(false);
                current_cell = current_cell_info.cell();
                return;
            }

            bn::regular_bg_map_cell_info left_up_cell_info(cells[map_item.cell_index(x - 1, y - 1)]);

            if(left_up_cell_info.tile_index() == ground_tile_index)
            {
                current_cell_info.set_tile_index(wall_bottom_cornor_tile_index);
                current_cell_info.set_palette_id(0);
                current_cell_info.set_horizontal_flip(true);
                current_cell = current_cell_info.cell();
                return;
            }
        }
    };

    void dynamic_affine_bg_scene()
    {
        // 禁用tile data自动偏移
        bn::bg_tiles::set_allow_offset(false);

        bn::unique_ptr<bg_map> bg_map_ptr(new bg_map());
        bn::affine_bg_item bg_item(
            bn::affine_bg_tiles_items::tiles, bn::bg_palette_items::palette, bg_map_ptr->map_item);
        bn::affine_bg_ptr bg = bg_item.create_bg(0, 0);
        bn::affine_bg_map_ptr bg_map = bg.map();

        // 启用
        bn::bg_tiles::set_allow_offset(true);

        int cursor_x = bg_map::columns / 2;
        int cursor_y = bg_map::rows / 2;
        bn::sprite_ptr cursor_sprite = bn::sprite_items::cursor.create_sprite(sprite_x(cursor_x), sprite_y(cursor_y));
        bg_map_ptr->dig(cursor_x, cursor_y);
        // notify Butano engine, cells data has been modified, update GBA VRAM
        bg_map.reload_cells_ref();

        while(1)
        {
            // 左键
            if(bn::keypad::left_pressed())
            {
                if(cursor_x > 4)
                {
                    --cursor_x;

                    if(bg_map_ptr->dig(cursor_x, cursor_y))
                    {
                        bg_map.reload_cells_ref();
                        // bg.set_scale(1.1);
                    }
                }
            }
            // 右键
            else if(bn::keypad::right_pressed())
            {
                if(cursor_x < bg_map::columns - 4 - 1)
                {
                    ++cursor_x;

                    if(bg_map_ptr->dig(cursor_x, cursor_y))
                    {
                        bg_map.reload_cells_ref();
                        // bg.set_scale(1.1);
                    }
                }
            }
            // 上键
            if(bn::keypad::up_pressed())
            {
                if(cursor_y > 10)
                {
                    --cursor_y;

                    if(bg_map_ptr->dig(cursor_x, cursor_y))
                    {
                        bg_map.reload_cells_ref();
                        // bg.set_scale(1.1);
                    }
                }
            }
            // 下键
            else if(bn::keypad::down_pressed())
            {
                if(cursor_y < bg_map::rows - 10 - 1)
                {
                    ++cursor_y;

                    if(bg_map_ptr->dig(cursor_x, cursor_y))
                    {
                        bg_map.reload_cells_ref();
                        // bg.set_scale(1.1);
                    }
                }
            }
            // 重置
            if(bn::keypad::start_pressed())
            {
                bg_map_ptr->reset();

                cursor_x = bg_map::columns / 2;
                cursor_y = bg_map::rows / 2;
                cursor_sprite.set_position(sprite_x(cursor_x), sprite_y(cursor_y));

                if(bg_map_ptr->dig(cursor_x, cursor_y))
                {
                    bg_map.reload_cells_ref();
                    // bg.set_scale(1.1);
                }
            }
            else
            {
                bn::fixed cursor_sprite_x = cursor_sprite.x();
                bn::fixed target_cursor_sprite_x = sprite_x(cursor_x);

                if(cursor_sprite_x < target_cursor_sprite_x)
                {
                    cursor_sprite.set_x(cursor_sprite_x + 1);
                }
                else if(cursor_sprite_x > target_cursor_sprite_x)
                {
                    cursor_sprite.set_x(cursor_sprite_x - 1);
                }

                bn::fixed cursor_sprite_y = cursor_sprite.y();
                bn::fixed target_cursor_sprite_y = sprite_y(cursor_y);

                if(cursor_sprite_y < target_cursor_sprite_y)
                {
                    cursor_sprite.set_y(cursor_sprite_y + 1);
                }
                else if(cursor_sprite_y > target_cursor_sprite_y)
                {
                    cursor_sprite.set_y(cursor_sprite_y - 1);
                }

                // bg.set_scale(bn::max(bg.horizontal_scale() - 0.05, bn::fixed(1)));
            }

            bn::core::update();
        }
    }

    void dynamic_regular_bg_scene()
    {
        bn::bg_tiles::set_allow_offset(false);

        bn::unique_ptr<regular_bg_map> bg_map_ptr(new regular_bg_map());
        bn::regular_bg_item bg_item(
            bn::regular_bg_tiles_items::tiles_2, bn::bg_palette_items::palette_2, bg_map_ptr->map_item);
        bn::regular_bg_ptr bg = bg_item.create_bg(0, 0);
        bn::regular_bg_map_ptr bg_map = bg.map();

        bn::bg_tiles::set_allow_offset(true);

        int cursor_x = regular_bg_map::columns / 2;
        int cursor_y = regular_bg_map::rows / 2;
        bn::sprite_ptr cursor_sprite = bn::sprite_items::cursor.create_sprite(sprite_x(cursor_x), sprite_y(cursor_y));
        bg_map_ptr->dig(cursor_x, cursor_y);
        bg_map.reload_cells_ref();
        // 临时设置，按键盘x键，下一个场景
        while(!bn::keypad::a_pressed())
        {
            if(bn::keypad::left_pressed())
            {
                if(cursor_x > 4)
                {
                    --cursor_x;

                    bg_map_ptr->dig(cursor_x, cursor_y);
                    bg_map.reload_cells_ref();
                }
            }
            else if(bn::keypad::right_pressed())
            {
                if(cursor_x < regular_bg_map::columns - 4 - 1)
                {
                    ++cursor_x;

                    bg_map_ptr->dig(cursor_x, cursor_y);
                    bg_map.reload_cells_ref();
                }
            }

            if(bn::keypad::up_pressed())
            {
                if(cursor_y > 10)
                {
                    --cursor_y;

                    bg_map_ptr->dig(cursor_x, cursor_y);
                    bg_map.reload_cells_ref();
                }
            }
            else if(bn::keypad::down_pressed())
            {
                if(cursor_y < regular_bg_map::rows - 10 - 1)
                {
                    ++cursor_y;

                    bg_map_ptr->dig(cursor_x, cursor_y);
                    bg_map.reload_cells_ref();
                }
            }

            if(bn::keypad::start_pressed())
            {
                bg_map_ptr->reset();

                cursor_x = regular_bg_map::columns / 2;
                cursor_y = regular_bg_map::rows / 2;
                cursor_sprite.set_position(sprite_x(cursor_x), sprite_y(cursor_y));

                bg_map_ptr->dig(cursor_x, cursor_y);
                bg_map.reload_cells_ref();
            }
            else
            {
                bn::fixed cursor_sprite_x = cursor_sprite.x();
                bn::fixed target_cursor_sprite_x = sprite_x(cursor_x);

                if(cursor_sprite_x < target_cursor_sprite_x)
                {
                    cursor_sprite.set_x(cursor_sprite_x + 1);
                }
                else if(cursor_sprite_x > target_cursor_sprite_x)
                {
                    cursor_sprite.set_x(cursor_sprite_x - 1);
                }

                bn::fixed cursor_sprite_y = cursor_sprite.y();
                bn::fixed target_cursor_sprite_y = sprite_y(cursor_y);

                if(cursor_sprite_y < target_cursor_sprite_y)
                {
                    cursor_sprite.set_y(cursor_sprite_y + 1);
                }
                else if(cursor_sprite_y > target_cursor_sprite_y)
                {
                    cursor_sprite.set_y(cursor_sprite_y - 1);
                }
            }

            bn::core::update();
        }
    }

    void map_collision_scene()
    {
        bn::regular_bg_ptr map_bg = bn::regular_bg_items::map.create_bg(0, 0);
        bn::sprite_ptr dog_sprite = bn::sprite_items::dog.create_sprite(0, 0);

        const bn::regular_bg_map_item& map_item = bn::regular_bg_items::map.map_item();
        bn::regular_bg_map_cell valid_map_cell = map_item.cell(0, 0);
        int valid_tile_index = bn::regular_bg_map_cell_info(valid_map_cell).tile_index();
        bn::point dog_map_position(16, 16);

        while(!bn::keypad::start_pressed())
        {
            bn::point new_dog_map_position = dog_map_position;

            if(bn::keypad::left_pressed())
            {
                new_dog_map_position.set_x(new_dog_map_position.x() - 1);
                dog_sprite.set_horizontal_flip(true);
            }
            else if(bn::keypad::right_pressed())
            {
                new_dog_map_position.set_x(new_dog_map_position.x() + 1);
                dog_sprite.set_horizontal_flip(false);
            }

            if(bn::keypad::up_pressed())
            {
                new_dog_map_position.set_y(new_dog_map_position.y() - 1);
            }
            else if(bn::keypad::down_pressed())
            {
                new_dog_map_position.set_y(new_dog_map_position.y() + 1);
            }

            bn::regular_bg_map_cell dog_map_cell = map_item.cell(new_dog_map_position);
            int dog_tile_index = bn::regular_bg_map_cell_info(dog_map_cell).tile_index();

            if(dog_tile_index == valid_map_cell)
            {
                dog_map_position = new_dog_map_position;
            }

            bn::fixed dog_sprite_x = (dog_map_position.x() * 8) - (map_item.dimensions().width() * 4) + 4;
            bn::fixed dog_sprite_y = (dog_map_position.y() * 8) - (map_item.dimensions().height() * 4) + 4;
            dog_sprite.set_position(dog_sprite_x, dog_sprite_y);

            bn::core::update();
        }
    }

    void sprite_mosaic_scene()
    {
        bn::sprite_ptr blonde_sprite = bn::sprite_items::blonde.create_sprite(0, 0);
        blonde_sprite.set_mosaic_enabled(true);

        bn::sprites_mosaic::set_stretch(0.1);

        while(!bn::keypad::start_pressed())
        {
            bn::fixed horizontal_stretch = bn::sprites_mosaic::horizontal_stretch();
            bn::fixed vertical_stretch = bn::sprites_mosaic::vertical_stretch();

            if(bn::keypad::left_held())
            {
                bn::sprites_mosaic::set_horizontal_stretch(bn::max(horizontal_stretch - 0.01, bn::fixed(0)));
            }
            else if(bn::keypad::right_held())
            {
                bn::sprites_mosaic::set_horizontal_stretch(bn::min(horizontal_stretch + 0.01, bn::fixed(1)));
            }

            if(bn::keypad::down_held())
            {
                bn::sprites_mosaic::set_vertical_stretch(bn::max(vertical_stretch - 0.01, bn::fixed(0)));
            }
            else if(bn::keypad::up_held())
            {
                bn::sprites_mosaic::set_vertical_stretch(bn::min(vertical_stretch + 0.01, bn::fixed(1)));
            }

            bn::core::update();
        }
        
        bn::sprites_mosaic::set_stretch(0);
    }

    void bgs_mosaic_scene()
    {
        bn::regular_bg_ptr land_2_bg = bn::regular_bg_items::land_2.create_bg(0, 0);
        land_2_bg.set_mosaic_enabled(true);

        bn::bgs_mosaic::set_stretch(0.1);

        while(!bn::keypad::start_pressed())
        {
            bn::fixed horizontal_stretch = bn::bgs_mosaic::horizontal_stretch();
            bn::fixed vertical_stretch = bn::bgs_mosaic::vertical_stretch();

            if(bn::keypad::left_held())
            {
                bn::bgs_mosaic::set_horizontal_stretch(bn::max(horizontal_stretch - 0.01, bn::fixed(0)));
            }
            else if(bn::keypad::right_held())
            {
                bn::bgs_mosaic::set_horizontal_stretch(bn::min(horizontal_stretch + 0.01, bn::fixed(1)));
            }

            if(bn::keypad::down_held())
            {
                bn::bgs_mosaic::set_vertical_stretch(bn::max(vertical_stretch - 0.01, bn::fixed(0)));
            }
            else if(bn::keypad::up_held())
            {
                bn::bgs_mosaic::set_vertical_stretch(bn::min(vertical_stretch + 0.01, bn::fixed(1)));
            }

            bn::core::update();
        }

        bn::bgs_mosaic::set_stretch(0);
    }
}

int main()
{
    bn::core::init();

    bn::sprite_text_generator text_generator(common::variable_8x16_sprite_font);
    bn::bg_palettes::set_transparent_color(bn::color(9, 16, 16));

    while(true)
    {
        bg_visiblity_scene(text_generator);
        bn::core::update();

        music_scene(text_generator);
        bn::core::update();

        land_scene();
        bn::core::update();

        // dynamic_affine_bg_scene();
        // bn::core::update();

        dynamic_regular_bg_scene();
        bn::core::update();

        map_collision_scene();
        bn::core::update();

        sprite_mosaic_scene();
        bn::core::update();

        bgs_mosaic_scene();
        bn::core::update();
    }
}
