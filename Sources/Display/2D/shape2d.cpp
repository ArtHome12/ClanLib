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
**    Harry Storbacka
**    Mark Page
*/

#include "Display/precomp.h"
#include "API/Display/2D/shape2d.h"
#include "shape2d_impl.h"
#include "API/Core/Math/ear_clip_result.h"
#include "API/Display/2D/canvas.h"
#include "API/Display/Font/font_vector.h"
#include "API/Core/Math/bezier_curve.h"
#include <vector>

namespace clan
{

/////////////////////////////////////////////////////////////////////////////
// Shape2D Construction:

Shape2D::Shape2D() : impl(new Shape2D_Impl())
{
}

Shape2D::~Shape2D()
{
}

/////////////////////////////////////////////////////////////////////////////
// Shape2D Attributes:


/////////////////////////////////////////////////////////////////////////////
// Shape2D Operations:

void Shape2D::add_path(Path2D &path)
{
	impl->add_path(path);
}

void Shape2D::get_triangles(std::vector<Vec2f> &out_primitives_array, PolygonOrientation orientation) const
{
	impl->get_triangles(out_primitives_array, orientation);
}

void Shape2D::get_outline(std::vector< std::vector<Vec2f> > &out_primitives_array_outline) const
{
	impl->get_outline(out_primitives_array_outline);
}

void Shape2D::add_circle(float center_x, float center_y, float radius, bool reverse)
{
	add_circle(Pointf(center_x, center_y), radius, reverse);

}

void Shape2D::add_circle(const Pointf &center, float radius, bool reverse)
{
	float offset_x = 0;
	float offset_y = 0;

	int rotationcount = max(5, (radius - 3));
	float halfpi = 1.5707963267948966192313216916398f;
	float turn = halfpi / rotationcount;

	offset_x = center.x;
	offset_y = -center.y;

	Path2D path;

	rotationcount *= 4;

	std::vector<Pointf> points;
	points.resize(rotationcount);

	for(int i = 0; i < rotationcount ; i++)
	{
		float pos1 = radius * cos(i * turn);
		float pos2 = radius * sin(i * turn);

		points[i].x = (center.x + pos1);
		points[i].y = (center.y + pos2);
	}

	path.add_line_to(points);

	if (reverse)
		path.reverse();

	add_path(path);
}

void Shape2D::add_rounded_rect(const Pointf &origin, const Sizef &size, float cap_rounding, bool reverse)
{
	// sanitize rounding values
	float tmp_rounding = cap_rounding;
	float min_rounding = min(size.width/2.0f, size.height/2.0f);
	if( tmp_rounding >= (min_rounding-0.01f) ) // 0.01: hysterezis for floating point errors
	{
		tmp_rounding = min_rounding-0.01f; // avoid duplicating curve endpoints 
	}

	// top right curve
	BezierCurve bez_tr;
	bez_tr.add_control_point(origin.x + size.width-tmp_rounding, origin.y);
	bez_tr.add_control_point(Pointf(origin.x + size.width, origin.y));
	bez_tr.add_control_point(origin.x + size.width, origin.y + tmp_rounding);

	// bottom right curve
	BezierCurve bez_br;
	bez_br.add_control_point(origin.x + size.width, origin.y + size.height-tmp_rounding);
	bez_br.add_control_point(Pointf(origin.x + size.width, origin.y + size.height));
	bez_br.add_control_point(origin.x + size.width-tmp_rounding, origin.y + size.height);
	
	// bottom left curve
	BezierCurve bez_bl;
	bez_bl.add_control_point(origin.x + tmp_rounding, origin.y + size.height);
	bez_bl.add_control_point(Pointf(origin.x, origin.y + size.height));
	bez_bl.add_control_point(origin.x, origin.y + size.height-tmp_rounding);

	// top left curve
	BezierCurve bez_tl;
	bez_tl.add_control_point(origin.x, origin.y +tmp_rounding);
	bez_tl.add_control_point(Pointf(origin.x, origin.y));
	bez_tl.add_control_point(origin.x + tmp_rounding, origin.y);
	
	Path2D path;

	path.add_curve(bez_tr);
	path.add_curve(bez_br);
	path.add_curve(bez_bl);
	path.add_curve(bez_tl);

	if (reverse)
		path.reverse();

	add_path(path);
}

/////////////////////////////////////////////////////////////////////////////
// Shape2D Implementation:

}
