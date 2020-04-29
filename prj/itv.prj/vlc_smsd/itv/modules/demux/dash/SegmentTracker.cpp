/*
 * SegmentTracker.cpp
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
#include "SegmentTracker.hpp"
#include "mpd/MPD.h"

using namespace dash;
using namespace dash::logic;
using namespace dash::http;
using namespace dash::mpd;

SegmentTracker::SegmentTracker(AbstractAdaptationLogic *logic_, mpd::MPD *mpd_)
{
    count = 0;
    initializing = true;
    prevRepresentation = NULL;
    currentPeriod = mpd_->getFirstPeriod();
    setAdaptationLogic(logic_);
    mpd = mpd_;
}

SegmentTracker::~SegmentTracker()
{

}

void SegmentTracker::setAdaptationLogic(AbstractAdaptationLogic *logic_)
{
    logic = logic_;
}

void SegmentTracker::resetCounter()
{
    count = 0;
    prevRepresentation = NULL;
}

Chunk * SegmentTracker::getNextChunk(Streams::Type type)
{
    Representation *rep;
    ISegment *segment;

    if(!currentPeriod)
        return NULL;

    if(prevRepresentation && !prevRepresentation->canBitswitch())
        rep = prevRepresentation;
    else
        rep = logic->getCurrentRepresentation(type, currentPeriod);

    if ( rep == NULL )
            return NULL;

    if(rep != prevRepresentation)
    {
        prevRepresentation = rep;
        initializing = true;
    }

    if(initializing)
    {
        initializing = false;
        segment = rep->getSegment(Representation::INFOTYPE_INIT);
        if(segment)
            return segment->toChunk(count, rep);
    }

    segment = rep->getSegment(Representation::INFOTYPE_MEDIA, count);
    if(!segment)
    {
        currentPeriod = mpd->getNextPeriod(currentPeriod);
        resetCounter();
        return getNextChunk(type);
    }

    Chunk *chunk = segment->toChunk(count, rep);
    if(chunk)
        count++;

    return chunk;
}

bool SegmentTracker::setPosition(mtime_t time, bool tryonly)
{
    uint64_t segcount;
    if(prevRepresentation &&
       prevRepresentation->getSegmentNumberByTime(time, &segcount))
    {
        if(!tryonly)
            count = segcount;
        return true;
    }
    return false;
}

mtime_t SegmentTracker::getSegmentStart() const
{
    if(prevRepresentation)
        return prevRepresentation->getPlaybackTimeBySegmentNumber(count);
    else
        return 0;
}
