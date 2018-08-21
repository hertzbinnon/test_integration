/*
 * IsoffMainParser.cpp
 *****************************************************************************
 * Copyright (C) 2010 - 2012 Klagenfurt University
 *
 * Created on: Jan 27, 2012
 * Authors: Christopher Mueller <christopher.mueller@itec.uni-klu.ac.at>
 *          Christian Timmerer  <christian.timmerer@itec.uni-klu.ac.at>
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "IsoffMainParser.h"
#include "SegmentTemplate.h"
#include "SegmentTimeline.h"
#include "ProgramInformation.h"
#include "xml/DOMHelper.h"
#include <vlc_strings.h>
#include <vlc_stream.h>
#include <cstdio>
#include <sstream>

using namespace dash::mpd;
using namespace dash::xml;

IsoffMainParser::IsoffMainParser    (Node *root, stream_t *p_stream) :
                 IMPDParser(root, NULL, p_stream, NULL)
{
}
IsoffMainParser::~IsoffMainParser   ()
{
}

bool    IsoffMainParser::parse              (Profile profile)
{
    mpd = new MPD(p_stream, profile);
    setMPDAttributes();
    parseProgramInformation(DOMHelper::getFirstChildElementByName(root, "ProgramInformation"), mpd);
    setMPDBaseUrl(root);
    parsePeriods(root);

    print();
    return true;
}

void    IsoffMainParser::setMPDAttributes   ()
{
    const std::map<std::string, std::string> attr = this->root->getAttributes();

    std::map<std::string, std::string>::const_iterator it;

    it = attr.find("mediaPresentationDuration");
    if(it != attr.end())
        this->mpd->duration.Set(IsoTime(it->second));

    it = attr.find("minBufferTime");
    if(it != attr.end())
        this->mpd->minBufferTime.Set(IsoTime(it->second));

    it = attr.find("minimumUpdatePeriod");
    if(it != attr.end())
        mpd->minUpdatePeriod.Set(IsoTime(it->second));

    it = attr.find("maxSegmentDuration");
    if(it != attr.end())
        mpd->maxSegmentDuration.Set(IsoTime(it->second));

    it = attr.find("type");
    if(it != attr.end())
        mpd->setType(it->second);

    it = attr.find("availabilityStartTime");
    if(it != attr.end())
        mpd->availabilityStartTime.Set(UTCTime(it->second));

    it = attr.find("timeShiftBufferDepth");
        if(it != attr.end())
            mpd->timeShiftBufferDepth.Set(IsoTime(it->second));
}

void IsoffMainParser::parsePeriods(Node *root)
{
    std::vector<Node *> periods = DOMHelper::getElementByTagName(root, "Period", false);
    std::vector<Node *>::const_iterator it;

    for(it = periods.begin(); it != periods.end(); it++)
    {
        Period *period = new (std::nothrow) Period(mpd);
        if (!period)
            continue;
        parseSegmentInformation(*it, period);
        if((*it)->hasAttribute("start"))
            period->startTime.Set(IsoTime((*it)->getAttributeValue("start")));
        if((*it)->hasAttribute("duration"))
            period->duration.Set(IsoTime((*it)->getAttributeValue("duration")));
        if((*it)->hasAttribute("id"))
            period->setId((*it)->getAttributeValue("id"));
        std::vector<Node *> baseUrls = DOMHelper::getChildElementByTagName(*it, "BaseURL");
        if(!baseUrls.empty())
            period->baseUrl.Set( new Url( baseUrls.front()->getText() ) );

        setAdaptationSets(*it, period);
        mpd->addPeriod(period);
    }
}

size_t IsoffMainParser::parseSegmentTemplate(Node *templateNode, SegmentInformation *info)
{
    size_t total = 0;
    if (templateNode == NULL || !templateNode->hasAttribute("media"))
        return total;

    std::string mediaurl = templateNode->getAttributeValue("media");
    MediaSegmentTemplate *mediaTemplate = NULL;
    if(mediaurl.empty() || !(mediaTemplate = new (std::nothrow) MediaSegmentTemplate(info)) )
        return total;
    mediaTemplate->setSourceUrl(mediaurl);

    if(templateNode->hasAttribute("startNumber"))
        mediaTemplate->startNumber.Set(Integer<uint64_t>(templateNode->getAttributeValue("startNumber")));

    if(templateNode->hasAttribute("duration"))
        mediaTemplate->duration.Set(Integer<mtime_t>(templateNode->getAttributeValue("duration")));

    if(templateNode->hasAttribute("timescale"))
        mediaTemplate->timescale.Set(Integer<uint64_t>(templateNode->getAttributeValue("timescale")));

    InitSegmentTemplate *initTemplate = NULL;

    if(templateNode->hasAttribute("initialization"))
    {
        std::string initurl = templateNode->getAttributeValue("initialization");
        if(!initurl.empty() && (initTemplate = new (std::nothrow) InitSegmentTemplate(info)))
            initTemplate->setSourceUrl(initurl);
    }

    mediaTemplate->initialisationSegment.Set(initTemplate);
    info->setSegmentTemplate(mediaTemplate);

    parseTimeline(DOMHelper::getFirstChildElementByName(templateNode, "SegmentTimeline"), mediaTemplate);

    total += ( mediaTemplate != NULL );

    return total;
}

size_t IsoffMainParser::parseSegmentInformation(Node *node, SegmentInformation *info)
{
    size_t total = 0;
    parseSegmentBase(DOMHelper::getFirstChildElementByName(node, "SegmentBase"), info);
    total += parseSegmentList(DOMHelper::getFirstChildElementByName(node, "SegmentList"), info);
    total += parseSegmentTemplate(DOMHelper::getFirstChildElementByName(node, "SegmentTemplate" ), info);
    if(node->hasAttribute("bitstreamSwitching"))
        info->setBitstreamSwitching(node->getAttributeValue("bitstreamSwitching") == "true");
    if(node->hasAttribute("timescale"))
        info->timescale.Set(Integer<uint64_t>(node->getAttributeValue("timescale")));
    return total;
}

void    IsoffMainParser::setAdaptationSets  (Node *periodNode, Period *period)
{
    std::vector<Node *> adaptationSets = DOMHelper::getElementByTagName(periodNode, "AdaptationSet", false);
    std::vector<Node *>::const_iterator it;

    for(it = adaptationSets.begin(); it != adaptationSets.end(); it++)
    {
        AdaptationSet *adaptationSet = new AdaptationSet(period);
        if(!adaptationSet)
            continue;
        if((*it)->hasAttribute("mimeType"))
            adaptationSet->setMimeType((*it)->getAttributeValue("mimeType"));

        parseSegmentInformation( *it, adaptationSet );

        setRepresentations((*it), adaptationSet);
        period->addAdaptationSet(adaptationSet);
    }
}
void    IsoffMainParser::setRepresentations (Node *adaptationSetNode, AdaptationSet *adaptationSet)
{
    std::vector<Node *> representations = DOMHelper::getElementByTagName(adaptationSetNode, "Representation", false);

    for(size_t i = 0; i < representations.size(); i++)
    {
        this->currentRepresentation = new Representation(adaptationSet, getMPD());
        Node *repNode = representations.at(i);

        std::vector<Node *> baseUrls = DOMHelper::getChildElementByTagName(repNode, "BaseURL");
        if(!baseUrls.empty())
            currentRepresentation->setBaseUrl( new BaseUrl( baseUrls.front()->getText() ) );

        if(repNode->hasAttribute("id"))
            currentRepresentation->setId(repNode->getAttributeValue("id"));

        if(repNode->hasAttribute("width"))
            this->currentRepresentation->setWidth(atoi(repNode->getAttributeValue("width").c_str()));

        if(repNode->hasAttribute("height"))
            this->currentRepresentation->setHeight(atoi(repNode->getAttributeValue("height").c_str()));

        if(repNode->hasAttribute("bandwidth"))
            this->currentRepresentation->setBandwidth(atoi(repNode->getAttributeValue("bandwidth").c_str()));

        if(repNode->hasAttribute("mimeType"))
            currentRepresentation->setMimeType(repNode->getAttributeValue("mimeType"));

        size_t totalmediasegments = parseSegmentInformation(repNode, currentRepresentation);
        if(!totalmediasegments)
        {
            /* unranged & segment less representation, add fake segment */
            SegmentList *list = new SegmentList();
            Segment *seg = new Segment(currentRepresentation);
            if(list && seg)
            {
                list->addSegment(seg);
                currentRepresentation->setSegmentList(list);
            }
            else
            {
                delete seg;
                delete list;
            }
        }

        adaptationSet->addRepresentation(this->currentRepresentation);
    }
}

void IsoffMainParser::parseSegmentBase(Node * segmentBaseNode, SegmentInformation *info)
{
    if(!segmentBaseNode)
        return;

    else if(segmentBaseNode->hasAttribute("indexRange"))
    {
        SegmentList *list = new SegmentList();
        Segment *seg;

        size_t start = 0, end = 0;
        if (std::sscanf(segmentBaseNode->getAttributeValue("indexRange").c_str(), "%zu-%zu", &start, &end) == 2)
        {
            seg = new IndexSegment(info);
            seg->setByteRange(start, end);
            list->addSegment(seg);
            /* index must be before data, so data starts at index end */
            seg = new Segment(info);
            seg->setByteRange(end + 1, 0);
        }
        else
        {
            seg = new Segment(info);
        }

        list->addSegment(seg);
        info->setSegmentList(list);

        Node *initSeg = DOMHelper::getFirstChildElementByName(segmentBaseNode, "Initialization");
        if(initSeg)
        {
            SegmentBase *base = new SegmentBase();
            parseInitSegment(initSeg, base);
            info->setSegmentBase(base);
        }
    }
    else
    {
        SegmentBase *base = new SegmentBase();
        parseInitSegment(DOMHelper::getFirstChildElementByName(segmentBaseNode, "Initialization"), base);
        info->setSegmentBase(base);
    }
}

size_t IsoffMainParser::parseSegmentList(Node * segListNode, SegmentInformation *info)
{
    size_t total = 0;
    mtime_t totaltime = 0;
    if(segListNode)
    {
        std::vector<Node *> segments = DOMHelper::getElementByTagName(segListNode, "SegmentURL", false);
        SegmentList *list;
        if(!segments.empty() && (list = new (std::nothrow) SegmentList()))
        {
            parseInitSegment(DOMHelper::getFirstChildElementByName(segListNode, "Initialization"), list);

            if(segListNode->hasAttribute("duration"))
                list->setDuration(Integer<mtime_t>(segListNode->getAttributeValue("duration")));

            if(segListNode->hasAttribute("timescale"))
                list->timescale.Set(Integer<uint64_t>(segListNode->getAttributeValue("timescale")));

            std::vector<Node *>::const_iterator it;
            for(it = segments.begin(); it != segments.end(); it++)
            {
                Node *segmentURL = *it;
                std::string mediaUrl = segmentURL->getAttributeValue("media");
                if(mediaUrl.empty())
                    continue;

                Segment *seg = new (std::nothrow) Segment(info);
                if(!seg)
                    continue;

                seg->setSourceUrl(segmentURL->getAttributeValue("media"));

                if(segmentURL->hasAttribute("mediaRange"))
                {
                    std::string range = segmentURL->getAttributeValue("mediaRange");
                    size_t pos = range.find("-");
                    seg->setByteRange(atoi(range.substr(0, pos).c_str()), atoi(range.substr(pos + 1, range.size()).c_str()));
                }

                if(totaltime || list->getDuration())
                {
                    seg->startTime.Set(totaltime);
                    totaltime += list->getDuration();
                }

                list->addSegment(seg);
                total++;
            }

            info->setSegmentList(list);
        }
    }
    return total;
}

void IsoffMainParser::parseInitSegment(Node *initNode, Initializable<Segment> *init)
{
    if(!initNode)
        return;

    Segment *seg = new InitSegment( currentRepresentation );
    seg->setSourceUrl(initNode->getAttributeValue("sourceURL"));

    if(initNode->hasAttribute("range"))
    {
        std::string range = initNode->getAttributeValue("range");
        size_t pos = range.find("-");
        seg->setByteRange(atoi(range.substr(0, pos).c_str()), atoi(range.substr(pos + 1, range.size()).c_str()));
    }

    init->initialisationSegment.Set(seg);
}

void IsoffMainParser::parseTimeline(Node *node, MediaSegmentTemplate *templ)
{
    if(!node)
        return;

    SegmentTimeline *timeline = new (std::nothrow) SegmentTimeline(templ);
    if(timeline)
    {
        std::vector<Node *> elements = DOMHelper::getElementByTagName(node, "S", false);
        std::vector<Node *>::const_iterator it;
        for(it = elements.begin(); it != elements.end(); it++)
        {
            const Node *s = *it;
            if(!s->hasAttribute("d")) /* Mandatory */
                continue;
            mtime_t d = Integer<mtime_t>(s->getAttributeValue("d"));
            mtime_t r = 0; // never repeats by default
            if(s->hasAttribute("r"))
                r = Integer<uint64_t>(s->getAttributeValue("r"));
            if(s->hasAttribute("t"))
            {
                mtime_t t = Integer<mtime_t>(s->getAttributeValue("t"));
                timeline->addElement(d, r, t);
            }
            else timeline->addElement(d, r);

            templ->segmentTimeline.Set(timeline);
        }
    }
}

void IsoffMainParser::parseProgramInformation(Node * node, MPD *mpd)
{
    if(!node)
        return;

    ProgramInformation *info = new (std::nothrow) ProgramInformation();
    if (info)
    {
        Node *child = DOMHelper::getFirstChildElementByName(node, "Title");
        if(child)
            info->setTitle(child->getText());

        child = DOMHelper::getFirstChildElementByName(node, "Source");
        if(child)
            info->setSource(child->getText());

        child = DOMHelper::getFirstChildElementByName(node, "Copyright");
        if(child)
            info->setCopyright(child->getText());

        if(node->hasAttribute("moreInformationURL"))
            info->setMoreInformationUrl(node->getAttributeValue("moreInformationURL"));

        mpd->programInfo.Set(info);
    }
}

void    IsoffMainParser::print              ()
{
    if(mpd)
    {
        msg_Dbg(p_stream, "MPD profile=%s mediaPresentationDuration=%ld minBufferTime=%ld",
                static_cast<std::string>(mpd->getProfile()).c_str(),
                mpd->duration.Get(),
                mpd->minBufferTime.Get());
        msg_Dbg(p_stream, "BaseUrl=%s", mpd->getUrlSegment().toString().c_str());

        std::vector<Period *>::const_iterator i;
        for(i = mpd->getPeriods().begin(); i != mpd->getPeriods().end(); i++)
        {
            std::vector<std::string> debug = (*i)->toString();
            std::vector<std::string>::const_iterator l;
            for(l = debug.begin(); l < debug.end(); l++)
            {
                msg_Dbg(p_stream, "%s", (*l).c_str());
            }
        }
    }
}

IsoTime::IsoTime(const std::string &str)
{
    time = str_duration(str.c_str());
}

IsoTime::operator mtime_t () const
{
    return time;
}

/* i_year: year - 1900  i_month: 0-11  i_mday: 1-31 i_hour: 0-23 i_minute: 0-59 i_second: 0-59 */
static int64_t vlc_timegm( int i_year, int i_month, int i_mday, int i_hour, int i_minute, int i_second )
{
    static const int pn_day[12+1] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
    int64_t i_day;

    if( i_year < 70 ||
        i_month < 0 || i_month > 11 || i_mday < 1 || i_mday > 31 ||
        i_hour < 0 || i_hour > 23 || i_minute < 0 || i_minute > 59 || i_second < 0 || i_second > 59 )
        return -1;

    /* Count the number of days */
    i_day = (int64_t)365 * (i_year-70) + pn_day[i_month] + i_mday - 1;
#define LEAP(y) ( ((y)%4) == 0 && (((y)%100) != 0 || ((y)%400) == 0) ? 1 : 0)
    for( int i = 70; i < i_year; i++ )
        i_day += LEAP(1900+i);
    if( i_month > 1 )
        i_day += LEAP(1900+i_year);
#undef LEAP
    /**/
    return ((24*i_day + i_hour)*60 + i_minute)*60 + i_second;
}

UTCTime::UTCTime(const std::string &str)
{
    enum { YEAR = 0, MON, DAY, HOUR, MIN, SEC, TZ };
    int values[7] = {0};
    std::istringstream in(str);

    try
    {
        /* Date */
        for(int i=YEAR;i<=DAY && !in.eof();i++)
        {
            if(i!=YEAR)
                in.ignore(1);
            in >> values[i];
        }
        /* Time */
        if (!in.eof() && in.peek() == 'T')
        {
            for(int i=HOUR;i<=SEC && !in.eof();i++)
            {
                in.ignore(1);
                in >> values[i];
            }
        }
        /* Timezone */
        if (!in.eof() && in.peek() == 'Z')
        {
            in.ignore(1);
            while(!in.eof())
            {
                if(in.peek() == '+')
                    continue;
                in >> values[TZ];
                break;
            }
        }

        time = vlc_timegm( values[YEAR] - 1900, values[MON] - 1, values[DAY],
                           values[HOUR], values[MIN], values[SEC] );
        time += values[TZ] * 3600;
    } catch(int) {
        time = 0;
    }
}

UTCTime::operator mtime_t () const
{
    return time;
}
