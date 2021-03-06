/* 
 * Copyright (C) 2014 WYSIWYD Consortium, European Commission FP7 Project ICT-612139
 * Authors: St�phane Lall�e
 * email:   stephane.lallee@gmail.com
 * website: http://efaa.upf.edu/ 
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

#include <wrdac/clients/icubClient.h>

using namespace std;
using namespace yarp::os;
using namespace yarp::sig;
using namespace yarp::dev;
using namespace wysiwyd::wrdac;


ICubClient::ICubClient(const std::string &moduleName, const std::string &context, const std::string &clientConfigFile, bool isRFVerbose)
{
    yarp::os::ResourceFinder rfClient;
    rfClient.setVerbose(isRFVerbose);
    rfClient.setDefaultContext(context.c_str());
    rfClient.setDefaultConfigFile(clientConfigFile.c_str());
    rfClient.configure(0,NULL);

    yarp::os::ResourceFinder rfPostures;
    rfPostures.setVerbose(isRFVerbose);
    rfPostures.setDefaultContext(context.c_str());
    rfPostures.setDefaultConfigFile(rfClient.check("posturesFile",Value("postures.ini")).asString().c_str());
    rfPostures.configure(0,NULL);

    yarp::os::ResourceFinder rfChoregraphies;
    rfChoregraphies.setVerbose(isRFVerbose);
    rfChoregraphies.setDefaultContext(context.c_str());
    rfChoregraphies.setDefaultConfigFile(rfClient.check("choregraphiesFile",Value("choregraphies.ini")).asString().c_str());
    rfChoregraphies.configure(0,NULL);

    LoadPostures(rfPostures);
    LoadChoregraphies(rfChoregraphies);

    //Reaching range
    Bottle defaultRangeMin ;defaultRangeMin.fromString("-0.5 -0.3 -0.15");
    Bottle defaultRangeMax;defaultRangeMax.fromString("-0.1 0.3 0.5");
    Bottle *rangeMin = rfClient.find("reachingRangeMin").asList();
    Bottle *rangeMax = rfClient.find("reachingRangeMax").asList();
    if (rangeMin == NULL) rangeMin = new Bottle(defaultRangeMin);
    if (rangeMax == NULL) rangeMax = new Bottle(defaultRangeMax);
    xRangeMin = defaultRangeMin.get(0).asDouble(); xRangeMax =defaultRangeMax.get(0).asDouble();
    yRangeMin = defaultRangeMin.get(1).asDouble(); yRangeMax =defaultRangeMax.get(1).asDouble();
    zRangeMin = defaultRangeMin.get(2).asDouble(); zRangeMax =defaultRangeMax.get(2).asDouble();

    icubAgent = NULL;

    //OPC
    string fullName = moduleName + "/icubClient";
    opc = new OPCClient(fullName);
    opc->isVerbose = false;

    //Susbsystems
    if (Bottle* bSubsystems = rfClient.find("subsystems").asList())
    {
        for (int s=0; s<bSubsystems->size(); s++)
        {
            std::string currentSS = bSubsystems->get(s).asString();
            cout<<"Trying to open subsystem : "<<currentSS<<endl;
            if (currentSS == SUBSYSTEM_ATTENTION)
                subSystems[SUBSYSTEM_ATTENTION] = new SubSystem_Attention(fullName);
            else if (currentSS == SUBSYSTEM_EXPRESSION)
                subSystems[SUBSYSTEM_EXPRESSION] = new SubSystem_Expression(fullName);
            else if (currentSS == SUBSYSTEM_POSTURES)
                subSystems[SUBSYSTEM_POSTURES] = new SubSystem_Postures(fullName);
            else if (currentSS == SUBSYSTEM_REACTABLE)
                subSystems[SUBSYSTEM_REACTABLE] = new SubSystem_Reactable(fullName);
            else if (currentSS == SUBSYSTEM_IKART)
                subSystems[SUBSYSTEM_IKART] = new SubSystem_iKart(fullName);
            else if (currentSS == SUBSYSTEM_ABM)
                subSystems[SUBSYSTEM_ABM] = new SubSystem_ABM(fullName);
            else if (currentSS == SUBSYSTEM_SPEECH)
                subSystems[SUBSYSTEM_SPEECH] = new SubSystem_Speech(fullName);
            else if (currentSS == SUBSYSTEM_SLIDING_CONTROLLER)
                subSystems[SUBSYSTEM_SLIDING_CONTROLLER] = new SubSystem_SlidingController(fullName);
            else if (currentSS == SUBSYSTEM_ARE)
                subSystems[SUBSYSTEM_ARE] = new SubSystem_ARE(fullName);
			else if (currentSS == SUBSYSTEM_RECOG)
                subSystems[SUBSYSTEM_RECOG] = new SubSystem_Recog(fullName);
        }
    }

    closed=false;
}


void ICubClient::LoadPostures(yarp::os::ResourceFinder &rf)
{
    posturesKnown.clear();

    int posCount = rf.check("posturesCount", yarp::os::Value(0)).asInt();
    //cout<<"Loading posture: "<<endl;
    for (int i = 0; i < posCount; i++)
    {
        std::stringstream ss;
        ss<<"posture_" << i;
        Bottle postureGroup = rf.findGroup(ss.str().c_str());
        BodyPosture p;
        std::string name = postureGroup.find("name").asString().c_str();
        //std::cout<<"\t"<<name<<std::endl;
        Bottle* bHead = postureGroup.find("head").asList();
        Bottle* bLArm = postureGroup.find("left_arm").asList();
        Bottle* bRArm = postureGroup.find("right_arm").asList();
        Bottle* bTorso = postureGroup.find("torso").asList();

        p.head.resize(6);
        for(int i=0;i<6;i++)
            p.head[i] = bHead->get(i).asDouble();
        p.left_arm.resize(16);
        for(int i=0;i<16;i++)
            p.left_arm[i] = bLArm->get(i).asDouble();        
        p.right_arm.resize(16);
        for(int i=0;i<16;i++)
            p.right_arm[i] = bRArm->get(i).asDouble();
        p.torso.resize(3);
        for(int i=0;i<3;i++)
            p.torso[i] = bTorso->get(i).asDouble();

        posturesKnown[name] = p;
    }
}


void ICubClient::LoadChoregraphies(yarp::os::ResourceFinder &rf)
{
    choregraphiesKnown.clear();

    int posCount = rf.check("choregraphiesCount", yarp::os::Value(0)).asInt();
    cout<<"Loading Choregraphies: "<<endl;
    for (int i = 0; i < posCount; i++)
    {
        std::stringstream ss;
        ss<<"chore_" << i;
        Bottle postureGroup = rf.findGroup(ss.str().c_str());

        std::string name = postureGroup.find("name").asString().c_str();
        std::cout<<"\t"<<name<<std::endl;
        Bottle* sequence = postureGroup.find("sequence").asList();

        std::list< std::pair<std::string, double> > seq;
        for(int s=0; s<sequence->size(); s++)
        {
            Bottle* element = sequence->get(s).asList();
            std::string elementName = element->get(0).asString().c_str();
            double elementTime = element->get(1).asDouble();
            seq.push_back(std::pair<std::string,double>(elementName,elementTime));
            //std::cout<<"\t \t"<<elementName<< "\t" << elementTime << std::endl;
        }
        choregraphiesKnown[name] = seq;
    }
}


bool ICubClient::connectOPC(const string &opcName)
{
    bool isConnected=opc->connect(opcName);
    if (isConnected)
        updateAgent();
    cout<<"Connection to OPC: "<<(isConnected?"successful":"failed")<<endl;
    return isConnected;
}


bool ICubClient::connectSubSystems()
{
    bool isConnected=true;
    for (map<string,SubSystem*>::iterator sIt=subSystems.begin(); sIt!=subSystems.end(); sIt++)
    {
        cout << "Connection to " << sIt->first << ": ";
        bool result=sIt->second->Connect();
        cout<<(result?"successful":"failed")<<endl;
        isConnected&=result;
    }
   
    return isConnected;
}


bool ICubClient::connect(const string &opcName)
{
    bool isConnected=connectOPC(opcName);
    isConnected&=connectSubSystems();
    return isConnected;
}


void ICubClient::close()
{
    if (closed)
        return;

    cout<<"Terminating subsystems:"<<endl; 
    for(map<string,SubSystem*>::iterator sIt = subSystems.begin();sIt!=subSystems.end();sIt++)
    {
        cout<<"\t"<<sIt->first<<endl;
        sIt->second->Close();
        delete sIt->second;
    }

    opc->interrupt();
    opc->close();
    delete opc;

    closed=true;
}


void ICubClient::updateAgent()
{
    if (opc->isConnected())
    {
        if (this->icubAgent == NULL)
        {
            icubAgent=opc->addAgent("icub");
        }
        else
            opc->update(this->icubAgent);
    }
    opc->Entities(EFAA_OPC_ENTITY_TAG, "==", EFAA_OPC_ENTITY_ACTION);
}


void ICubClient::commitAgent()
{       
    if (opc->isConnected())
        opc->commit(this->icubAgent);
}
    
    
bool ICubClient::moveToPosture(const string &name, double time)
{
    if (subSystems.find("postures") == subSystems.end())
    {
        cout<<"Impossible, postures system is not running..."<<endl;
        return false;
    }

    if (posturesKnown.find(name) == posturesKnown.end())
    {
        cout<<"Unknown posture"<<endl;
        return false;
    }

    ((SubSystem_Postures*) subSystems["postures"])->Execute(posturesKnown[name], time);
    return true;
}


bool ICubClient::moveBodyPartToPosture(const string &name, double time, const string &bodyPart)
{
    if (subSystems.find("postures") == subSystems.end())
    {
        cout<<"Impossible, postures system is not running..."<<endl;
        return false;
    }

    if (posturesKnown.find(name) == posturesKnown.end())
    {
        cout<<"Unknown posture"<<endl;
        return false;
    }

    ((SubSystem_Postures*) subSystems["postures"])->Execute(posturesKnown[name], time, bodyPart);
    return true;
}

    
bool ICubClient::playBodyPartChoregraphy(const std::string &name, const std::string &bodyPart, double speedFactor,  bool isBlocking )
{
    if (choregraphiesKnown.find(name) == choregraphiesKnown.end())
    {
        cout<<"Unknown choregraphy"<<endl;
        return false;
    }
     bool overallError = true;
    cout<<"Playing "<<name<<" at "<<speedFactor<<" speed"<<endl;    
    std::list< std::pair<std::string, double> > chore = choregraphiesKnown[name];
    for( std::list< std::pair<std::string, double> >::iterator element = chore.begin() ; element != chore.end() ; element++)
    {
        double factoredTime = element->second / speedFactor;
        cout<<"Going to "<<element->first<<" in "<<factoredTime<<endl;
        overallError &= moveBodyPartToPosture(element->first,factoredTime,bodyPart);

        if (isBlocking)
            Time::delay(factoredTime);
    }
    return overallError;
}


double ICubClient::getChoregraphyLength(const std::string &name, double speedFactor)
{
    if (choregraphiesKnown.find(name) == choregraphiesKnown.end())
    {
        cout<<"Unknown choregraphy"<<endl;
        return 0.0;
    }
    double totalTime = 0.0;
    std::list< std::pair<std::string, double> > chore = choregraphiesKnown[name];
    for( std::list< std::pair<std::string, double> >::iterator element = chore.begin() ; element != chore.end() ; element++)
        totalTime += element->second / speedFactor;
    
    cout<<"Playing "<<name<<" at "<<speedFactor<<" speed should take "<<endl;   
    return totalTime;
}


bool ICubClient::playChoregraphy(const std::string &name, double speedFactor, bool isBlocking)
{
    if (choregraphiesKnown.find(name) == choregraphiesKnown.end())
    {
        cout<<"Unknown choregraphy"<<endl;
        return false;
    }
     bool overallError = true;
    cout<<"Playing "<<name<<" at "<<speedFactor<<" speed"<<endl;    
    std::list< std::pair<std::string, double> > chore = choregraphiesKnown[name];
    for( std::list< std::pair<std::string, double> >::iterator element = chore.begin() ; element != chore.end() ; element++)
    {
        double factoredTime = element->second / speedFactor;
        cout<<"Going to "<<element->first<<" in "<<factoredTime<<endl;
        overallError &= moveToPosture(element->first,factoredTime);

        if (isBlocking)
            Time::delay(factoredTime);
    }
    return overallError;
}


bool ICubClient::goTo(const string &place)
{
    cerr << "Try to call \"gotTo\" on iCubClient but the method is not implemented." << endl;
    return false;
}


bool ICubClient::home(const string &part)
{
    SubSystem_ARE *are=getARE();
    if (are==NULL)
    {
        cerr<<"[iCubClient] Called home() but ARE subsystem is not available."<<endl;
        return false;
    }

    return are->home(part);
}


bool ICubClient::grasp(const string &oName, const Bottle &options)
{
    Entity *target=opc->getEntity(oName,true);
    if (!target->isType(EFAA_OPC_ENTITY_OBJECT))
    {
        cerr<<"[iCubClient] Called grasp() on a unallowed entity: \""<<oName<<"\""<<endl;
        return false;
    }

    Object *oTarget=dynamic_cast<Object*>(target);
    if (!oTarget->m_present)
    {
        cerr<<"[iCubClient] Called grasp() on an unavailable entity: \""<<oName<<"\""<<endl;
        return false;
    }

    return grasp(oTarget->m_ego_position,options);
}


bool ICubClient::grasp(const Vector &target, const Bottle &options)
{
    SubSystem_ARE *are=getARE();
    if (are==NULL)
    {
        cerr<<"[iCubClient] Called grasp() but ARE subsystem is not available."<<endl;
        return false;
    }

    if (isTargetInRange(target))
    {
        Bottle opt(options);
        opt.addString("still"); // always avoid automatic homing after grasp
        return are->take(target,opt);
    }
    else
    {
        cerr<<"[iCubClient] Called grasp() on a unreachable entity: ("<<target.toString(3,3).c_str()<<")"<<endl;
        return false;
    }
}


bool ICubClient::release(const string &oLocation, const Bottle &options)
{
    Entity *target=opc->getEntity(oLocation,true);
    if (!target->isType(EFAA_OPC_ENTITY_RTOBJECT) && !target->isType(EFAA_OPC_ENTITY_OBJECT))
    {
        cerr<<"[iCubClient] Called release() on a unallowed location: \""<<oLocation<<"\""<<endl;
        return false;
    }

    Object *oTarget=dynamic_cast<Object*>(target);
    if (!oTarget->m_present)
    {
        cerr<<"[iCubClient] Called release() on an unavailable entity: \""<<oLocation<<"\""<<endl;
        return false;
    }

    return release(oTarget->m_ego_position,options);
}


bool ICubClient::release(const Vector &target, const Bottle &options)
{
    SubSystem_ARE *are=getARE();
    if (are==NULL)
    {
        cerr<<"[iCubClient] Called release() but ARE subsystem is not available."<<endl;
        return false;
    }

    if (isTargetInRange(target))
        return are->dropOn(target,options);
    else
    {
        cerr<<"[iCubClient] Called release() on a unreachable location: ("<<target.toString(3,3).c_str()<<")"<<endl;
        return false;
    }
}


bool ICubClient::point(const string &oLocation, const Bottle &options)
{
    Entity *target=opc->getEntity(oLocation,true);
    if (!target->isType(EFAA_OPC_ENTITY_RTOBJECT) && !target->isType(EFAA_OPC_ENTITY_OBJECT))
    {
        cerr<<"[iCubClient] Called point() on a unallowed location: \""<<oLocation<<"\""<<endl;
        return false;
    }

    Object *oTarget=dynamic_cast<Object*>(target);
    if (!oTarget->m_present)
    {
        cerr<<"[iCubClient] Called point() on an unavailable entity: \""<<oLocation<<"\""<<endl;
        return false;
    }

    return point(oTarget->m_ego_position,options);
}


bool ICubClient::point(const Vector &target, const Bottle &options)
{
    SubSystem_ARE *are=getARE();
    if (are==NULL)
    {
        cerr<<"[iCubClient] Called point() but ARE subsystem is not available."<<endl;
        return false;
    }

    Bottle opt(options);
    opt.addString("still"); // always avoid automatic homing after point
    return are->point(target,opt);
}


bool ICubClient::look(const string &target)
{        
    if (subSystems.find("attention") == subSystems.end())
    {
        cout<<"Impossible, attention is not running..."<<endl;
        return false;
    }
    Bottle cmd, reply;
    cmd.addString("track");
    cmd.addString(target.c_str());
    ((SubSystem_Attention*) subSystems["attention"])->attentionSelector.write(cmd,reply);
    return (reply.get(0).asVocab() == VOCAB3('a','c','k'));
}


bool ICubClient::lookAround()
{        
    if (subSystems.find("attention") == subSystems.end())
    {
        cout<<"Impossible, attention is not running..."<<endl;
        return false;
    }
    Bottle cmd, reply;
    cmd.addString("auto");
    ((SubSystem_Attention*) subSystems["attention"])->attentionSelector.write(cmd,reply);
    return (reply.get(0).asVocab() == VOCAB3('a','c','k'));
}


bool ICubClient::lookStop()
{        
    if (subSystems.find("attention") == subSystems.end())
    {
        cout<<"Impossible, attention is not running..."<<endl;
        return false;
    }
    Bottle cmd, reply;
    cmd.addString("sleep");
    ((SubSystem_Attention*) subSystems["attention"])->attentionSelector.write(cmd,reply);
    return (reply.get(0).asVocab() == VOCAB3('a','c','k'));
}


void ICubClient::getHighestEmotion(string &emotionName, double &intensity)
{
    intensity = 0.0;
    emotionName = "joy";
    
    //cout<<"EMOTIONS : "<<endl;
    for(map<string,double>::iterator d=this->icubAgent->m_emotions_intrinsic.begin(); d != this->icubAgent->m_emotions_intrinsic.end(); d++)
    {
        //cout<<'\t'<<d->first<<'\t'<<d->second<<endl;
        if ( d->second > intensity )
        {
            emotionName = d->first;
            intensity = d->second;
        }
    }
}


bool ICubClient::say(const string &text, bool shouldWait, bool emotionalIfPossible, const std::string &overrideVoice)
{        
    if (subSystems.find("speech") == subSystems.end())
    {
        cout<<"Impossible, speech is not running..."<<endl;
        return false;
    }

    if (subSystems.find("expression") != subSystems.end() && emotionalIfPossible && subSystems["speech"]->getType() == SUBSYSTEM_SPEECH_ESPEAK)
    {
        string emo;
        double value;
        getHighestEmotion(emo,value);
        this->getExpressionClient()->express(emo,value,(SubSystem_Speech_eSpeak*) subSystems["speech"],overrideVoice);
    }

    ((SubSystem_Speech*) subSystems["speech"])->TTS(text,shouldWait);
    return true;
}


bool ICubClient::execute(Action &what, bool applyEstimatedDriveEffect)
{
    cout<<"iCubClient>> Executing plan: "<<what.toString()<<endl;
    bool overallResult = true;
    list<Action> unrolled = what.asPlan();
    for(list<Action>::iterator a = unrolled.begin(); a != unrolled.end(); a++)
    {        
        bool result = true;
        cout<<"iCubClient>> Executing action: "<<a->toString()<<endl;

        //First we check if the iCub should do this or if it is a someone else
        if (a->description().subject() == "icub")
        {
            //At this level all actions are primitive. We check if it is known or not
            if (a->name() == "say")
                result = say(a->description().object());
            else if (a->name() == "go-to")
                result = goTo(a->description().object());
            else if (a->name() == "grasp")
                result = grasp(a->description().object());
            else if (a->name() == "release")
                result = release(a->description().object());
            else 
            {
                cout<<"Warning: " << a->description().verb() << " is not composite, however it is not a primitive"<<endl;
                result = true;
            }
        }
        else
        {
            //todo wait for an action of the user
            result = say("I should wait until");
            result = say(a->description().toString());
        }
        overallResult = overallResult && result;

        //Apply the estimated effect for each subaction
        if (applyEstimatedDriveEffect /*&& result*/)
        {
            for(map<string,double>::iterator effect = a->estimatedDriveEffects.begin() ; effect != a->estimatedDriveEffects.end() ; effect++)
            {
                this->icubAgent->m_drives[effect->first].value += effect->second;
                this->icubAgent->m_drives[effect->first].value = max(0.0, min(1.0, this->icubAgent->m_drives[effect->first].value));
                this->commitAgent();
            }
        }

        //If the action failed we wait 5s. FOR DEBUG PURPOSE
        if (!result)
        {
            cout<<"Action failed... Waiting 5s"<<endl;
            Time::delay(5.0);
        }
    }

    //Apply the estimated effect for the general plan
    if (applyEstimatedDriveEffect /*&& result*/)
    {
        for(map<string,double>::iterator effect = what.estimatedDriveEffects.begin() ; effect != what.estimatedDriveEffects.end() ; effect++)
        {
            this->icubAgent->m_drives[effect->first].value += effect->second;
            this->icubAgent->m_drives[effect->first].value = max(0.0, min(1.0, this->icubAgent->m_drives[effect->first].value));
            this->commitAgent();
        }
    }
    return overallResult;
}


list<Action*> ICubClient::getKnownActions()
{        
    this->actionsKnown.clear();
    list<Entity*> entities = opc->Entities(EFAA_OPC_ENTITY_TAG,"==",EFAA_OPC_ENTITY_ACTION);
    for(list<Entity*>::iterator it = entities.begin(); it!= entities.end() ; it++)
    {
        actionsKnown.push_back( (Action*) (*it) );
    }
    return actionsKnown;
}


list<Object*> ICubClient::getObjectsInSight()
{
    list<Object*> inSight;
    opc->checkout();
    list<Entity*> allEntities = opc->EntitiesCache();
    for(list<Entity*>::iterator it = allEntities.begin(); it!= allEntities.end() ; it++)
    {
        if ( (*it)->isType(EFAA_OPC_ENTITY_OBJECT) )
        {
            Vector itemPosition = this->icubAgent->getSelfRelativePosition( ((Object*)(*it))->m_ego_position );

            //For now we just test if the object is in front of the robot
            if (itemPosition[0] < 0)
                inSight.push_back((Object*)(*it));
        }
    }
    return inSight;
}


list<Object*> ICubClient::getObjectsInRange()
{
    //float sideReachability = 0.3f; //30cm on each side
    //float frontCloseReachability = -0.1f; //from 10cm in front of the robot
    //float frontFarReachability = -0.3f; //up to 30cm in front of the robot

    list<Object*> inRange;
    opc->checkout();
    list<Entity*> allEntities = opc->EntitiesCache();
    for(list<Entity*>::iterator it = allEntities.begin(); it!= allEntities.end() ; it++)
    {
        if ( (*it)->isType(EFAA_OPC_ENTITY_OBJECT) && ((Object*)(*it))->m_present)
        {
            Vector itemPosition = this->icubAgent->getSelfRelativePosition( ((Object*)(*it))->m_ego_position );

            if (isTargetInRange(itemPosition))
                inRange.push_back((Object*)(*it));
        }
    }
    return inRange;
}


bool ICubClient::isTargetInRange(const Vector &target) const
{
    //cout<<"Target current root position is : "<<target.toString(3,3)<<endl;
    //cout<<"Range is : \n"
    //    <<"\t x in ["<<xRangeMin<<" ; "<<xRangeMax<<"]\n"
    //    <<"\t y in ["<<yRangeMin<<" ; "<<yRangeMax<<"]\n"
    //    <<"\t z in ["<<zRangeMin<<" ; "<<zRangeMax<<"]\n";

    bool isIn = ((target[0]>xRangeMin) && (target[0]<xRangeMax) &&
                 (target[1]>yRangeMin) && (target[1]<yRangeMax) && 
                 (target[2]>zRangeMin) && (target[2]<zRangeMax));
    //cout<<"Target in range = "<<isIn<<endl;

    return isIn;
}   

