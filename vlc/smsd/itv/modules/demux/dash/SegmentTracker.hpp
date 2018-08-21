/*
 * SegmentTracker.hpp
 *****************************************************************************
 * Copyright (C) 2014 - VideoLAN authors
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/
#ifndef SEGMENTTRACKER_HPP
#define SEGMENTTRACKER_HPP

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "StreamsType.hpp"
#include <vlc_common.h>

namespace dash
{
    namespace mpd
    {
        class MPD;
        class Period;
        class Representation;
        class Segment;
    }

    namespace logic
    {
        class AbstractAdaptationLogic;
    }

    namespace http
    {
        class Chunk;
    }

    class SegmentTracker
    {
        public:
            SegmentTracker(logic::AbstractAdaptationLogic *, mpd::MPD *);
            ~SegmentTracker();

            void setAdaptationLogic(logic::AbstractAdaptationLogic *);
            void resetCounter();
            http::Chunk* getNextChunk(Streams::Type);
            bool setPosition(mtime_t, bool);
            mtime_t getSegmentStart() const;

        private:
            bool initializing;
            uint64_t count;
            logic::AbstractAdaptationLogic *logic;
            mpd::MPD *mpd;
            mpd::Period *currentPeriod;
            mpd::Representation *prevRepresentation;
    };
}

#endif // SEGMENTTRACKER_HPP
