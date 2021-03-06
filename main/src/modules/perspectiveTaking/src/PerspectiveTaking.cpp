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

#include <stdio.h>

#include <QApplication>
#include <QTimer>
#include <QThread>
#include <QtCore/QMetaType>

#include <opencv2/core/core.hpp>
#include <boost/chrono/chrono.hpp>

#include <yarp/math/Math.h>
#include <Eigen/Dense>
#include <pcl/common/common_headers.h>

#include <rtabmap/core/Parameters.h>

#include "MapBuilder.h"
#include "PerspectiveTaking.h"

using namespace std;
using namespace yarp::os;
using namespace yarp::sig;
using namespace yarp::math;
using namespace kinectWrapper;
using namespace wysiwyd::wrdac;

cv::Mat perspectiveTaking::lastDepth;

bool perspectiveTaking::configure(yarp::os::ResourceFinder &rf) {
    resfind = rf;
    setName(rf.check("name", Value("perspectiveTaking"), "module name (string)").asString().c_str());
    loopCounter = 0;
    isConnectedToAgentDetector = false;
    isConnectedToOPC = false;
    isConnectedToABM = false;

    // connect to the various other modules
    connectToOPC(rf.check("opcName",Value("OPC")).asString().c_str());
    connectToKinectServer(rf.check("kinClientVerbosity",Value(0)).asInt());
    connectToABM(rf.check("abmName",Value("autobiographicalMemory")).asString().c_str());
    connectToAgentDetector(rf.check("agentDetectorName",Value("agentDetector")).asString().c_str());
    connectToHeadPoseEstimator(rf.check("headPoseEstimatorName",Value("headPoseEstimator")).asString().c_str());

    int partnerCameraMode_temp = rf.check("partnerCameraMode",Value(0)).asInt();
    if(partnerCameraMode_temp==0) {
        partnerCameraMode = staticPos;
    } else if(partnerCameraMode_temp==1) {
        if(isConnectedToAgentDetector && isConnectedToOPC) {
            partnerCameraMode = agentDetector;
        } else {
            cerr << "Asked to use agentDetector to set camera view, but agentDetector/OPC is not running " << endl;
            cerr << "Using static position instead!" << endl;
            partnerCameraMode = staticPos;
        }
    } else if(partnerCameraMode_temp==2) {
        if(isConnectedToHeadPoseEstimator) {
            partnerCameraMode = headPose;
        } else {
            cerr << "Asked to use headPoseEstimator to set camera view, but it is not running " << endl;
            cerr << "Using static position instead!" << endl;
            partnerCameraMode = staticPos;
        }
    } else {
        cerr << "Camera mode not supported, abort!" << endl;
        return false;
    }

    openHandlerPort();

    //kinect2robot_pcl = getRFHTransMat(resfind.check("rfhName",Value("referenceFrameHandler")).asString().c_str());
    kinect2robot_pcl = getManualTransMat(rf.check("cameraOffsetX",Value(0.0)).asDouble(),
                      rf.check("cameraOffsetZ",Value(-0.4)).asDouble(),
                      rf.check("cameraAngle",Value(-20.0)).asDouble());
    cout << "Kinect 2 Robot PCL: " << endl << kinect2robot_pcl << endl;

    // connect to ABM
    selfPerspImgPort.open("/"+getName()+"/images/self:o");
    partnerPerspImgPort.open("/"+getName()+"/images/partner:o");
    addABMImgProvider(selfPerspImgPort.getName(), true);
    addABMImgProvider(partnerPerspImgPort.getName(), true);

    // create QApplication and threads
    int argc_qt = 1;
    char *argv_qt[] = {(char*)getName().c_str()};
    QApplication app(argc_qt, argv_qt);

    mapBuilder = new MapBuilder(rf.check("decimationOdometry",Value(2)).asInt(),
                                rf.check("decimationStatistics",Value(2)).asInt());

    setupThreads();

    // we need to convert from yarp reference frame to pcl reference frame
    yarp2pcl = Eigen::Matrix4f::Identity();
    yarp2pcl(0, 0) = -1; // x is back in yarp, forward in pcl
    yarp2pcl(1, 1) = -1; // y is right in yarp, left in pcl

    // Set camera position for robot viewpoint
    Eigen::Vector4f robot_pos  = Eigen::Vector4f( 0,0,0,1);
    Eigen::Vector4f robot_view = Eigen::Vector4f(-1,0,0,1);
    Eigen::Vector4f robot_up   = Eigen::Vector4f( 0,0,1,1);
    setViewCameraReference(robot_pos, robot_view, robot_up, "robot");
    //setViewRobotReference(robot_pos, robot_view, robot_up, "robot");

    // set field of view for cameras
    float fovy_human = rf.check("fovyHuman",Value(135)).asInt();
    float fovy_robot = rf.check("fovyCamera",Value(58)).asInt();

    mapBuilder->setCameraFieldOfView(fovy_robot/180.0*3.14, "robot");
    mapBuilder->setCameraFieldOfView(fovy_human/180.0*3.14, "partner");

    distanceMultiplier = rf.check("distanceMultiplier",Value(1.0)).asDouble();

    // start new thread to update position of the partner camera
    setCamPosThread = new QThread(this);
    setCamPosTimer = new QTimer(0);

    setCamPosTimer->setInterval(rf.check("updateTimer",Value(1000)).asInt());
    setCamPosTimer->moveToThread(setCamPosThread);
    connect(setCamPosTimer, SIGNAL(timeout()), this, SLOT(setPartnerCamera()));
    QObject::connect(setCamPosThread, SIGNAL(started()), setCamPosTimer, SLOT(start()));
    setCamPosThread->start();

    // start the QApplication and go in a loop
    mapBuilder->show();
    app.exec();

    return true;
}

bool perspectiveTaking::respond(const Bottle& cmd, Bottle& reply) {
    if (cmd.get(0).asString() == "learnEnvironment") {
        // TODO: Not yet implemented
        //cout << getName() << ": Going to learn the environment" << endl;
        reply.addString("nack");
    } else if (cmd.get(0).asString() == "setUpdateTimer") {
        if(cmd.get(1).isInt()) {
            setCamPosTimer->setInterval(cmd.get(1).asInt());
            //headPoseTimer->setInterval(cmd.get(1).asInt());
            reply.addString("ack");
        } else {
            reply.addString("Wrong function call: setUpdateTimer 1000");
        }
    } else if (cmd.get(0).asString() == "setDecimationOdometry") {
        if(cmd.get(1).isInt()) {
            mapBuilder->setDecimationOdometry(cmd.get(1).asInt());
            reply.addString("ack");
        } else {
            reply.addString("Wrong function call: setDecimationOdometry 2");
        }
    } else if (cmd.get(0).asString() == "setDecimationStatistics") {
        if(cmd.get(1).isInt()) {
            mapBuilder->setDecimationStatistics(cmd.get(1).asInt());
            reply.addString("ack");
        } else {
            reply.addString("Wrong function call: setDecimationStatistics 2");
        }
    } else if (cmd.get(0).asString() == "processStats") {
        if(cmd.get(1).isInt()) {
            mapBuilder->doProcessStats = cmd.get(1).asInt() > 0;
            reply.addString("ack");
        } else {
            reply.addString("Wrong function call: processStats 0/1");
        }
    } else {
        reply.addString("nack");
    }
    return true;
}

bool perspectiveTaking::sendImagesToPorts() {
    cv::Mat screen = mapBuilder->getScreen();

    cv::Mat selfPersp, partnerPersp;
    // self perspective = left side of screenshot
    // partner perspective = right side of screenshot
    selfPersp=screen(cv::Rect(0,0,screen.cols/2,screen.rows));
    partnerPersp=screen(cv::Rect(screen.cols/2,0,screen.cols/2,screen.rows));
    //cv::imshow("Self Perspective", selfPersp);
    //cv::imshow("Partner Perspective", partnerPersp);
    //cv::waitKey(50);

    // convert cv::Mat to IplImage, and copy IplImage to ImageOf<PixelRGB>
    IplImage* partnerPersp_ipl = new IplImage(partnerPersp);
    ImageOf<PixelRgb> &partnerPers_yarp = partnerPerspImgPort.prepare();
    partnerPers_yarp.resize(partnerPersp_ipl->width, partnerPersp_ipl->height);
    cvCopyImage(partnerPersp_ipl, (IplImage *)partnerPers_yarp.getIplImage());

    IplImage* selfPersp_ipl = new IplImage(selfPersp);
    ImageOf<PixelRgb> &selfPers_yarp = selfPerspImgPort.prepare();
    selfPers_yarp.resize(selfPersp_ipl->width, selfPersp_ipl->height);
    cvCopyImage(selfPersp_ipl, (IplImage *)selfPers_yarp.getIplImage());

    //send the images
    selfPerspImgPort.write();
    partnerPerspImgPort.write();

    return true;
}

void perspectiveTaking::setPartnerCamera() {
    loopCounter++;

    Eigen::Matrix4f odometryPos = mapBuilder->getLastOdomPose().toEigen4f();

    if(partnerCameraMode==staticPos) {
        double d_headPos[3] = {-1.624107, -0.741913, 0.590235};
        Vector p_headPos = Vector(3, d_headPos);

        double d_up[3] = {p_headPos[0], p_headPos[1], p_headPos[2]+1.0};
        Vector p_up = Vector(3, d_up);

        double d_shoulderLeft[3] = {-1.617043, -0.620365, 0.356680};
        Vector p_shoulderLeft  = Vector(3, d_shoulderLeft);

        double d_shoulderRight[3] = {-1.601168, -0.888877, 0.352823};
        Vector p_shoulderRight = Vector(3, d_shoulderRight);

        Vector p_view = distanceMultiplier * yarp::math::cross(p_shoulderRight-p_headPos, p_shoulderLeft-p_headPos);
        p_view[2] = p_view[2] - 0.8;

        // take odometry pose into account
        Eigen::Vector4f pos = odometryPos * yarp2EigenV(p_headPos);
        Eigen::Vector4f view = odometryPos * yarp2EigenV(p_view);
        Eigen::Vector4f up = odometryPos * yarp2EigenV(p_up);

        cout << "Call setPartnerCamera" << endl;
        setViewRobotReference(pos, view, up, "partner");

        sendImagesToPorts();
    } else if(partnerCameraMode==agentDetector) {
        if(!isConnectedToOPC) {
            return;
        }
        partner = dynamic_cast<Agent*>( opc->getEntity("partner", true) );
        if(partner) { // && partner->m_present
            Vector p_headPos = partner->m_ego_position;
            Vector p_shoulderLeft = partner->m_body.m_parts["shoulderLeft"];
            Vector p_shoulderRight = partner->m_body.m_parts["shoulderRight"];

            double d_up[3] = {p_headPos[0], p_headPos[1], p_headPos[2]+1.0};
            Vector p_up = Vector(3, d_up);

            // For now, the partner is thought to look towards the robot
            // This is achieved by laying a plane between left shoulder,
            // right shoulder and head and using the normal vector of
            // the plane as viewing vector
            Vector p_view = distanceMultiplier * yarp::math::cross(p_shoulderRight-p_headPos, p_shoulderLeft-p_headPos);
            p_view[2] = p_view[2] - lookDown;

            // take odometry pose into account
            Eigen::Vector4f pos = odometryPos * yarp2EigenV(p_headPos);
            Eigen::Vector4f view = odometryPos * yarp2EigenV(p_view);
            Eigen::Vector4f up = odometryPos * yarp2EigenV(p_up);

            cout << "Call setPartnerCamera" << endl;
            setViewRobotReference(pos, view, up, "partner");

            sendImagesToPorts();
        } else {
            cout << "No partner present!" << endl;
        }
    } else if(partnerCameraMode==headPose) {
        cout << "Partner camera mode = headPose" << endl;
        if(!isConnectedToHeadPoseEstimator) {
            return;
        }

        // TODO: move to separate method!
        Bottle bCmd, bReply;
        bCmd.addString("getPoses");
        headPoseEstimator.write(bCmd, bReply);

        const int pose_size = 6;

        vector< cv::Vec<float,pose_size> > g_means;

        if(bReply.get(0).toString()!="[ack]") {
            cout << "Did not get [ack]" << endl;
            return;
        } else {
            int size = bReply.get(1).asInt();
            for(int i=0;i<size;i++) {
                cv::Vec<float,pose_size> mean;
                for(int j=0; j<pose_size; j++) {
                    // + 2 as 0->[ack], 1->size() of vector, 2-> actual begin of message
                    cout << i << " " << j << bReply.get(i*pose_size+j+2).asDouble() << endl;
                    mean[j] = bReply.get(i*pose_size+j+2).asDouble();
                }
                g_means.push_back(mean);
            }
        }

        mapBuilder->getVisualizerByName("robot")->getVisualizer().removeAllShapes();

        if(g_means.size()) {
            cout << "Received pose, set camera" << endl;

            Eigen::Matrix3f m_rotation (Eigen::AngleAxisf(pcl::deg2rad(g_means[0][3]), Eigen::Vector3f::UnitX())
                                       * Eigen::AngleAxisf(pcl::deg2rad(g_means[0][4]), Eigen::Vector3f::UnitY())
                                       * Eigen::AngleAxisf(pcl::deg2rad(g_means[0][5]), Eigen::Vector3f::UnitZ()));

            Eigen::Vector3f g_face_curr_dir = m_rotation*Eigen::Vector3f(0,0,-1); // (0,0,1) = g_face_dir

            // -75.0 because g_means in the center of the head -> we want to have the nose tip instead
            Eigen::Vector3f head_center = Eigen::Vector3f(g_means[0][0],
                                                          g_means[0][1],
                                                          g_means[0][2] - 75.0);
            Eigen::Vector3f head_front = head_center + 550.d*g_face_curr_dir;

            Eigen::Vector4f pos = odometryPos*Eigen::Vector4f(head_center[2]/1000.0, -head_center[0]/1000.0, -head_center[1]/1000.0, 1);
            Eigen::Vector4f up = pos; up(2) += 1.0;
            Eigen::Vector4f view = odometryPos*Eigen::Vector4f(head_front[2]/1000.0, -head_front[0]/1000.0, -head_front[1]/1000.0, 1);

            //cout << "Odom: " << endl << odometryPos << endl << endl;
            //cout << "Pos: "  << endl << pos  << endl << endl;
            //cout << "View: " << endl << view << endl << endl;

            pcl::PointXYZ begin(pos[0],  pos[1],  pos[2]);
            pcl::PointXYZ end(view[0], view[1], view[2]);

            // to undo the yarp->pcl stuff, which is applied in setViewCameraReference
            pos[0]  = -pos[0];     pos[1] = -pos[1];
            up[0]   = -up[0];       up[1] = -up[1];
            view[0] = -view[0];   view[1] = -view[1];

            mapBuilder->getVisualizerByName("robot")->getVisualizer().addArrow(end, begin, 0.0, 1.0, 0.0, "faceArrow");

            cout << "Call setPartnerCamera" << endl;
            setViewCameraReference(pos, view, up, "partner");

            sendImagesToPorts();
        }
    } else {
        cerr << "This partner camera mode is not supported!" << endl;
    }

    // print some statistics
    /*
    if(rtabmap->getLoopClosureId()) {
        printf(" #%ld ptime(%fs) STM(%ld) WM(%ld) hyp(%d) value(%.2f) *LOOP %d->%d*\n",
               loopCounter,
               rtabmap->getLastProcessTime(),
               rtabmap->getSTM().size(), // short-term memory
               rtabmap->getWM().size(), // working memory
               rtabmap->getLoopClosureId(),
               rtabmap->getLoopClosureValue(),
               rtabmap->getLastLocationId(),
               rtabmap->getLoopClosureId());
    }
    else {
        printf(" #%ld ptime(%fs) STM(%ld) WM(%ld) hyp(%d) value(%.2f)\n",
               loopCounter,
               rtabmap->getLastProcessTime(),
               rtabmap->getSTM().size(), // short-term memory
               rtabmap->getWM().size(), // working memory
               rtabmap->getHighestHypothesisId(), // highest loop closure hypothesis
               rtabmap->getLoopClosureValue());
    }*/
}

bool perspectiveTaking::close() {
    // remove handlers
    mapBuilder->unregisterFromEventsManager();
    rtabmapThread->unregisterFromEventsManager();
    odomThread->unregisterFromEventsManager();

    // close methods
    // remove ABM image providers
    addABMImgProvider(selfPerspImgPort.getName(), false);
    addABMImgProvider(partnerPerspImgPort.getName(), false);

    boost::this_thread::sleep_for(boost::chrono::milliseconds(100));

    // Kill all threads
    setCamPosThread->quit();
    setCamPosThread->wait();

    cameraThread->join(true);
    odomThread->join(true);
    rtabmapThread->join(true);

    // generate graph and save Long-Term Memory
    rtabmap->generateDOTGraph(resfind.findFileByName("Graph.dot"));
    printf("Generated graph \"Graph.dot\", viewable with Graphiz using \"neato -Tpdf Graph.dot -o out.pdf\"\n");

    // Cleanup... save database and logs
    printf("Saving Long-Term Memory to \"rtabmap.db\"...\n");
    rtabmap->close();

    // Delete pointers
    //delete mapBuilder; //is deleted by QVTKWidget destructor!
    delete cameraThread;
    delete rtabmapThread;
    delete odomThread;

    // Close ports
    abm.interrupt();
    abm.close();
    selfPerspImgPort.interrupt();
    selfPerspImgPort.close();
    partnerPerspImgPort.interrupt();
    partnerPerspImgPort.close();
    opc->interrupt();
    opc->close();
    handlerPort.interrupt();
    handlerPort.close();
    client.close();

    return true;
}
