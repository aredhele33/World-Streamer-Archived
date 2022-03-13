#pragma once

// Note : Using doubles to avoid precision issues is never a good idead
//		  but it wasn't a concern for this small project.
struct SpaceReferencePrecise
{
	double x;	///< The x position
	double y;	///< The y position
	double vx;	///< The velocity on x (Only for rendering)
	double vy;	///< The velocity on y (Only for rendering)
};