/*
 *
 * Copyright (C) 2015 WYSIWYD Consortium, European Commission FP7 Project ICT-612139
 * Authors: Tobias Fischer
 * email:   t.fischer@imperial.ac.uk
 * Permission is granted to copy, distribute, and/or modify this program
 * under the terms of the GNU General Public License, version 2 or any
 * later version published by the Free Software Foundation.
 *
 * A copy of the license can be found at
 * wysiwyd/license/gpl.txt
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details
*/

#ifndef VPT_MAPBUILDER
#define VPT_MAPBUILDER

#include <QVBoxLayout>
#include <QtCore/QMetaType>

#include <opencv2/core/core.hpp>

#include <rtabmap/core/OdometryEvent.h>
#include <rtabmap/core/RtabmapEvent.h>
#include <rtabmap/utilite/UEventsHandler.h>

#include "VisualizerWrapper.h"

using namespace rtabmap;

// This class receives RtabmapEvent and construct/update a 3D Map
class MapBuilder : public QWidget, public UEventsHandler {
    Q_OBJECT
public:
    MapBuilder(unsigned int decOdo, unsigned int decVis);
    virtual ~MapBuilder();

    bool wasStopped();

    bool doProcessStats;

    cv::Mat getScreen();

    void setCameraPosition( double pos_x, double pos_y, double pos_z,
                            double view_x, double view_y, double view_z,
                            double up_x, double up_y, double up_z,
                            const std::string &visualizerName );

    void setCameraFieldOfView(double fovy, const std::string &visualizerName );

    void setDecimationOdometry(int decimation) {
        decimationOdometry_ = decimation;
    }

    void setDecimationStatistics(int decimation) {
        decimationStatistics_ = decimation;
    }

    VisualizerWrapper* getVisualizerByName(const std::string &name) {
        VisualizerWrapper* vis = NULL;
        if(name == "robot") {
            vis = _vWrapper_robot;
        } else if(name == "partner") {
            vis = _vWrapper_partner;
        } else {
            cerr << "No visualizer with name " << name << endl;
        }
        return vis;
    }

    Transform getLastOdomPose() { return lastOdomPose_; }

private slots:
    void processOdometry(const rtabmap::SensorData & data);
    void processStatistics(const rtabmap::Statistics & stats);

protected:
    virtual void handleEvent(UEvent * event);

private:
    VisualizerWrapper* _vWrapper_robot;
    VisualizerWrapper* _vWrapper_partner;
    // decimation to show points clouds; the higher, the lower the resolution
    unsigned int decimationOdometry_; // odometry: current point cloud decimation
    unsigned int decimationStatistics_; // statistics: past point cloud decimation
    boost::recursive_try_mutex vis_mutex;

    Transform lastOdomPose_;
    Transform odometryCorrection_;
};

#endif
