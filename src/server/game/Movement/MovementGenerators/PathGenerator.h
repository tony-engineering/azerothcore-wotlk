/*
 * Copyright (C) 2016+     AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-GPL2
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _PATH_GENERATOR_H
#define _PATH_GENERATOR_H

#include "SharedDefines.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshQuery.h"
#include "MoveSplineInitArgs.h"
#include "MMapFactory.h"
#include "MMapManager.h"

class Unit;

// 74*4.0f=296y  number_of_points*interval = max_path_len
// this is way more than actual evade range
// I think we can safely cut those down even more
#define MAX_PATH_LENGTH             74
#define MAX_POINT_PATH_LENGTH       74

#define SMOOTH_PATH_STEP_SIZE       4.0f
#define SMOOTH_PATH_SLOP            0.3f
#define ALLOWED_DIST_FROM_POLY      2.5f
#define ADDED_Z_FOR_POLY_LOOKUP     0.3f
#define DISALLOW_TIME_AFTER_FAIL    3 // secs
#define MAX_FIXABLE_Z_ERROR         7.0f

#define VERTEX_SIZE       3
#define INVALID_POLYREF   0

enum PathType
{
    PATHFIND_BLANK          = 0x00,   // path not built yet
    PATHFIND_NORMAL         = 0x01,   // normal path
    PATHFIND_SHORTCUT       = 0x02,   // travel through obstacles, terrain, air, etc (old behavior)
    PATHFIND_INCOMPLETE     = 0x04,   // we have partial path to follow - getting closer to target
    PATHFIND_NOPATH         = 0x08,   // no valid path at all or error in generating one
    PATHFIND_NOT_USING_PATH = 0x10,   // used when we are either flying/swiming or on map w/o mmaps
    PATHFIND_SHORT          = 0x20,   // path is longer or equal to its limited path length
};

class PathGenerator
{
    public:
        explicit PathGenerator(WorldObject const* owner);
        ~PathGenerator();

        // Calculate the path from owner to given destination
        // return: true if new path was calculated, false otherwise (no change needed)
        bool CalculatePath(float destX, float destY, float destZ, bool forceDest = false);
        bool CalculatePath(float x, float y, float z, float destX, float destY, float destZ, bool forceDest);
        [[nodiscard]] bool IsInvalidDestinationZ(Unit const* target) const;
        [[nodiscard]] bool IsWalkableClimb(float const* v1, float const* v2) const;
        [[nodiscard]] bool IsWalkableClimb(float x, float y, float z, float destX, float destY, float destZ, float sourceHeight) const;
        [[nodiscard]] bool IsWalkableClimb(float x, float y, float z, float destX, float destY, float destZ) const;
        [[nodiscard]] float GetRequiredHeightToClimb(float x, float y, float z, float destX, float destY, float destZ, float sourceHeight) const;

        // option setters - use optional

        // when set, it skips paths with too high slopes (doesn't work with StraightPath enabled)
        void SetSlopeCheck(bool checkSlope) { _slopeCheck = checkSlope; }
        void SetUseStraightPath(bool useStraightPath) { _useStraightPath = useStraightPath; }
        void SetPathLengthLimit(float distance) { _pointPathLimit = std::min<uint32>(uint32(distance/SMOOTH_PATH_STEP_SIZE), MAX_POINT_PATH_LENGTH); }
        void SetUseRaycast(bool useRaycast) { _useRaycast = useRaycast; }

        // result getters
        G3D::Vector3 const& GetStartPosition() const { return _startPosition; }
        G3D::Vector3 const& GetEndPosition() const { return _endPosition; }
        G3D::Vector3 const& GetActualEndPosition() const { return _actualEndPosition; }

        Movement::PointsArray const& GetPath() const { return _pathPoints; }

        PathType GetPathType() const { return _type; }

        // shortens the path until the destination is the specified distance from the target point
        void ShortenPathUntilDist(G3D::Vector3 const& point, float dist);

        float getPathLength() const
        {
            dx = _pathPoints[0].x - _startPosition.x;
            dy = _pathPoints[0].y - _startPosition.y;
            dz = _pathPoints[0].z - _startPosition.z;
            len += sqrt( dx * dx + dy * dy + dz * dz );
        }
        else
            return len;
        for (uint32 i = 1; i < size; ++i)
        {
            dx = _pathPoints[i].x - _pathPoints[i - 1].x;
            dy = _pathPoints[i].y - _pathPoints[i - 1].y;
            dz = _pathPoints[i].z - _pathPoints[i - 1].z;
            len += sqrt( dx * dx + dy * dy + dz * dz );
        }
        return len;
    }

private:

    dtPolyRef _pathPolyRefs[MAX_PATH_LENGTH];   // array of detour polygon references
    uint32 _polyLength;                         // number of polygons in the path

        bool _useStraightPath;  // type of path will be generated (do not use it for movement paths)
        bool _forceDestination; // when set, we will always arrive at given point
        bool _slopeCheck;       // when set, it skips paths with too high slopes (doesn't work with _useStraightPath)
        uint32 _pointPathLimit; // limit point path size; min(this, MAX_POINT_PATH_LENGTH)
        bool _useRaycast;       // use raycast if true for a straight line path

    bool _useStraightPath;  // type of path will be generated
    bool _forceDestination; // when set, we will always arrive at given point
    uint32 _pointPathLimit; // limit point path size; min(this, MAX_POINT_PATH_LENGTH)

    G3D::Vector3 _startPosition;        // {x, y, z} of current location
    G3D::Vector3 _endPosition;          // {x, y, z} of the destination
    G3D::Vector3 _actualEndPosition;    // {x, y, z} of the closest possible point to given destination

        dtQueryFilterExt _filter;  // use single filter for all movements, update it when needed

    dtQueryFilter _filter;  // use single filter for all movements, update it when needed

    void SetStartPosition(G3D::Vector3 const& point) { _startPosition = point; }
    void SetEndPosition(G3D::Vector3 const& point) { _actualEndPosition = point; _endPosition = point; }
    void SetActualEndPosition(G3D::Vector3 const& point) { _actualEndPosition = point; }

    void Clear()
    {
        _polyLength = 0;
        _pathPoints.clear();
    }

    bool InRange(G3D::Vector3 const& p1, G3D::Vector3 const& p2, float r, float h) const;
    float Dist3DSqr(G3D::Vector3 const& p1, G3D::Vector3 const& p2) const;
    bool InRangeYZX(float const* v1, float const* v2, float r, float h) const;

    dtPolyRef GetPathPolyByPosition(dtPolyRef const* polyPath, uint32 polyPathSize, float const* Point, float* Distance = nullptr) const;
    dtPolyRef GetPolyByLocation(float* Point, float* Distance) const;
    bool HaveTile(G3D::Vector3 const& p) const;

    void BuildPolyPath(G3D::Vector3 const& startPos, G3D::Vector3 const& endPos, ACE_RW_Thread_Mutex& lock);
    void BuildPointPath(float const* startPoint, float const* endPoint);
    void BuildShortcut();

    NavTerrain GetNavTerrain(float x, float y, float z);
    void CreateFilter();
    void UpdateFilter();

    // smooth path aux functions
    uint32 FixupCorridor(dtPolyRef* path, uint32 npath, uint32 maxPath, dtPolyRef const* visited, uint32 nvisited);
    bool GetSteerTarget(float const* startPos, float const* endPos, float minTargetDist, dtPolyRef const* path, uint32 pathSize, float* steerPos,
                        unsigned char& steerPosFlag, dtPolyRef& steerPosRef);
    dtStatus FindSmoothPath(float const* startPos, float const* endPos,
                            dtPolyRef const* polyPath, uint32 polyPathSize,
                            float* smoothPath, int* smoothPathSize, uint32 smoothPathMaxSize);
};

#endif
