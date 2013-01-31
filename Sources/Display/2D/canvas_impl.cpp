/*
**  ClanLib SDK
**  Copyright (c) 1997-2012 The ClanLib Team
**
**  This software is provided 'as-is', without any express or implied
**  warranty.  In no event will the authors be held liable for any damages
**  arising from the use of this software.
**
**  Permission is granted to anyone to use this software for any purpose,
**  including commercial applications, and to alter it and redistribute it
**  freely, subject to the following restrictions:
**
**  1. The origin of this software must not be misrepresented; you must not
**     claim that you wrote the original software. If you use this software
**     in a product, an acknowledgment in the product documentation would be
**     appreciated but is not required.
**  2. Altered source versions must be plainly marked as such, and must not be
**     misrepresented as being the original software.
**  3. This notice may not be removed or altered from any source distribution.
**
**  Note: Some of the libraries ClanLib may link to may have additional
**  requirements or restrictions.
**
**  File Author(s):
**
**    Magnus Norddahl
**    Mark Page
*/

#include "Display/precomp.h"
#include "canvas_impl.h"
#include "API/Display/Render/render_batcher.h"
#include "API/Display/Render/shared_gc_data.h"
#include "API/Display/TargetProviders/graphic_context_provider.h"
#include "API/Display/2D/gradient.h"

namespace clan
{

Canvas_Impl::Canvas_Impl(GraphicContext &gc)
	: gc(gc), active_batcher(0), canvas_map_mode(map_user_projection)
{
	font_manager = SharedGCData::get_font_manager();

	gc_clip_z_range = gc.get_provider()->get_clip_z_range();
	canvas_modelviews.push_back(Mat4f::identity());

	if (gc.get_write_frame_buffer().is_null())	// No framebuffer attached to canvas
	{
		canvas_y_axis = y_axis_top_down;
		slot_window_resized = gc.get_provider()->sig_window_resized().connect(this, &Canvas_Impl::on_window_resized);
	}
	else
	{
		if (gc.get_texture_image_y_axis() == y_axis_bottom_up)
		{
			canvas_y_axis = y_axis_bottom_up;
		}
		else
		{
			canvas_y_axis = y_axis_top_down;
		}
	}

	update_viewport_size();
}

Canvas_Impl::~Canvas_Impl()
{
	if (!gc.is_null())
		flush();
}

RenderBatchTriangle *Canvas_Impl::get_triangle_batcher()
{
	if (!render_batcher_triangle)
		render_batcher_triangle = std::shared_ptr<RenderBatchTriangle>(new RenderBatchTriangle(&render_batcher_buffer));
	return render_batcher_triangle.get();
}

RenderBatchLine *Canvas_Impl::get_line_batcher()
{
	if (!render_batcher_line)
		render_batcher_line = std::shared_ptr<RenderBatchLine>(new RenderBatchLine(&render_batcher_buffer));
	return render_batcher_line.get();
}

RenderBatchLineTexture *Canvas_Impl::get_line_texture_batcher()
{
	if (!render_batcher_line_texture)
		render_batcher_line_texture = std::shared_ptr<RenderBatchLineTexture>(new RenderBatchLineTexture(&render_batcher_buffer));
	return render_batcher_line_texture.get();
}

RenderBatchPoint *Canvas_Impl::get_point_batcher()
{
	if (!render_batcher_point)
		render_batcher_point = std::shared_ptr<RenderBatchPoint>(new RenderBatchPoint(&render_batcher_buffer));
	return render_batcher_point.get();
}

void Canvas_Impl::flush()
{
	if (active_batcher)
	{
		RenderBatcher *batcher = active_batcher;
		active_batcher = 0;
		batcher->flush(gc);
	}
}

void Canvas_Impl::update_batcher_matrix()
{
	if (active_batcher)
	{
		active_batcher->matrix_changed(canvas_modelviews.back(), canvas_projection);
	}
}

void Canvas_Impl::set_batcher(Canvas &canvas, RenderBatcher *batcher)
{
	if (active_batcher != batcher)
	{
		flush();
		active_batcher = batcher;
		update_batcher_matrix();
	}
}

void Canvas_Impl::calculate_map_mode_matrices()
{
	Mat4f matrix;

	MapMode mode = (canvas_y_axis == y_axis_bottom_up) ? get_top_down_map_mode() : canvas_map_mode;
	switch (mode)
	{
	default:
	case map_2d_upper_left:
		matrix = Mat4f::ortho_2d(0.0f, canvas_size.width, canvas_size.height, 0.0f, handed_right, gc_clip_z_range);
		break;
	case map_2d_lower_left:
		matrix = Mat4f::ortho_2d(0.0f, canvas_size.width, 0.0f, canvas_size.height, handed_right, gc_clip_z_range);
		break;
	case map_user_projection:
		matrix = user_projection;
		break;
	}
	if (matrix != canvas_projection)
	{
		canvas_projection = matrix;
		update_batcher_matrix();
	}

}

MapMode Canvas_Impl::get_top_down_map_mode() const
{
	switch (canvas_map_mode)
	{
	default:
	case map_2d_upper_left: return map_2d_lower_left;
	case map_2d_lower_left: return map_2d_upper_left;
	case map_user_projection: return map_user_projection;
	}
}

void Canvas_Impl::set_modelview(const Mat4f &modelview)
{
	canvas_modelviews.back() = modelview;
	update_batcher_matrix();
}

void Canvas_Impl::push_modelview(const Mat4f &modelview)
{
	canvas_modelviews.push_back(modelview);
	update_batcher_matrix();
}

void Canvas_Impl::pop_modelview()
{
	canvas_modelviews.pop_back();

	if (canvas_modelviews.empty())
		throw Exception("Popped modelview too many times");

	update_batcher_matrix();
}

const Mat4f &Canvas_Impl::get_modelview() const
{
	return canvas_modelviews.back();
}

const Mat4f &Canvas_Impl::get_projection() const
{
	return canvas_projection;
}

void Canvas_Impl::set_map_mode(MapMode map_mode)
{
	canvas_map_mode = map_mode;
	calculate_map_mode_matrices();
}

void Canvas_Impl::set_user_projection(const Mat4f &projection)
{
	user_projection = projection;
	calculate_map_mode_matrices();
}

void Canvas_Impl::update_viewport_size()
{
	Size size = gc.get_size();
	if (size != canvas_size)
	{
		canvas_size = size;
		calculate_map_mode_matrices();
	}
}

void Canvas_Impl::clear(const Colorf &color)
{
	gc.clear(color);
}

void Canvas_Impl::on_window_resized(const Size &size)
{
	update_viewport_size();
}

void Canvas_Impl::write_cliprect(const Rect &rect)
{
	gc.set_scissor(rect, canvas_y_axis ? y_axis_top_down : y_axis_bottom_up);
}

void Canvas_Impl::set_cliprect(const Rect &rect)
{
	if (!cliprects.empty())
		cliprects.back() = rect;
	else
		cliprects.push_back(rect);
	write_cliprect(rect);
}

void Canvas_Impl::push_cliprect(const Rect &rect)
{
	if (!cliprects.empty())
	{
		Rect r = cliprects.back();
		r.overlap(rect);
		cliprects.push_back(r);
	}
	else
	{
		cliprects.push_back(rect);
	}

	write_cliprect(cliprects.back());
}

void Canvas_Impl::push_cliprect()
{
	if (cliprects.empty())
	{
		cliprects.push_back(gc.get_size());
	}
	else
	{
		cliprects.push_back(cliprects.back());
	}
	write_cliprect(cliprects.back());
}

void Canvas_Impl::pop_cliprect()
{
	if (cliprects.empty())
		throw Exception("GraphicContext::pop_cliprect - popped too many times!");

	cliprects.pop_back();

	if (cliprects.empty())
		reset_cliprect();
	else
		write_cliprect(cliprects.back());
}

void Canvas_Impl::reset_cliprect()
{
	cliprects.clear();
	gc.reset_scissor();
}

void Canvas_Impl::get_gradient_colors(const Vec2f *triangles, int num_vertex, const Gradient &gradient, std::vector<Colorf> &out_colors)
{
	out_colors.clear();
	out_colors.reserve(num_vertex);
	if (num_vertex)
	{
		Rectf bounding_box = get_triangles_bounding_box(triangles, num_vertex);
		Sizef bounding_box_size = bounding_box.get_size();
		if (bounding_box_size.width <= 0.0f)
			bounding_box_size.width = 1.0f;
		if (bounding_box_size.height <= 0.0f)
			bounding_box_size.height = 1.0f;

		Sizef bounding_box_invert_size( 1.0f / bounding_box_size.width, 1.0f / bounding_box_size.height );

		for(;num_vertex>0; --num_vertex)
		{
			Vec2f point = *(triangles++);
			point.x -= bounding_box.left;
			point.y -= bounding_box.top;
			point.x *= bounding_box_invert_size.width;
			point.y *= bounding_box_invert_size.height;

			Colorf top_color(
				(gradient.top_right.r * point.x) + (gradient.top_left.r * (1.0f - point.x)),
				(gradient.top_right.g * point.x) + (gradient.top_left.g * (1.0f - point.x)),
				(gradient.top_right.b * point.x) + (gradient.top_left.b * (1.0f - point.x)),
				(gradient.top_right.a * point.x) + (gradient.top_left.a * (1.0f - point.x)) );

			Colorf bottom_color(
				(gradient.bottom_right.r * point.x) + (gradient.bottom_left.r * (1.0f - point.x)),
				(gradient.bottom_right.g * point.x) + (gradient.bottom_left.g * (1.0f - point.x)),
				(gradient.bottom_right.b * point.x) + (gradient.bottom_left.b * (1.0f - point.x)),
				(gradient.bottom_right.a * point.x) + (gradient.bottom_left.a * (1.0f - point.x)) );

			Colorf color(
				(bottom_color.r * point.y) + (top_color.r * (1.0f - point.y)), 
				(bottom_color.g * point.y) + (top_color.g * (1.0f - point.y)), 
				(bottom_color.b * point.y) + (top_color.b * (1.0f - point.y)), 
				(bottom_color.a * point.y) + (top_color.a * (1.0f - point.y)) );
				
			out_colors.push_back(color);
		}
	}
}

Rectf Canvas_Impl::get_triangles_bounding_box(const Vec2f *triangles, int num_vertex)
{
	Rectf bounding_box;
	if (num_vertex)
		bounding_box = Rectf(triangles->x, triangles->y, triangles->x, triangles->y);

	for (--num_vertex; num_vertex > 0; --num_vertex)
	{
		triangles++;
		bounding_box.left = min(triangles->x, bounding_box.left);
		bounding_box.top = min(triangles->y, bounding_box.top);
		bounding_box.right = max(triangles->x, bounding_box.right);
		bounding_box.bottom = max(triangles->y, bounding_box.bottom);
	}
	return bounding_box;
}

}

