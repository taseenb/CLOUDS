//
//  CloudsAct.cpp
//  CloudsStoryEngine
//
//  Created by Surya Mattu on 04/07/13.
//
//

#include "CloudsAct.h"

CloudsAct::CloudsAct(){
    currentPlayIndex = 0;
    startTime = 0;
    actPlaying =false;
    waitingForNextClip = false;
    //TODO: Make non-arbritrary
    visualSystemDuration = 60;
    duration = 0;
    timeLineActive = false;
}

void CloudsAct::loadNextClip(){
    currentClip = clips[currentPlayIndex];
    currentTopic = topicHistory[currentPlayIndex];
    currentPlayIndex++;
    
    CloudsStoryEventArgs args(currentClip,currentTopic);
    ofNotifyEvent(events.clipBegan, args, this);
}


void CloudsAct::playAct(){
    currentPlayIndex = 0;
    currentClip = clips[currentPlayIndex];
    currentTopic = topicHistory[currentPlayIndex];
    startTime = timer.getAppTimeSeconds();
    actPlaying = true;
    clipEndTime = startTime +currentClip.getDuration();
    
    //secsSinceLastVisualSystemPlayed =0;
    CloudsStoryEventArgs argsA(currentClip,currentTopic);
    ofNotifyEvent(events.actBegan,argsA ,this);
    
}

void CloudsAct::populateTime(){

    timeline.setup();
    timeline.clear();
    
    timeline.setDurationInSeconds(duration);
    timeline.setAutosave(false);
    
    visualSystemsTrack = timeline.addFlags("Visual Systems");
    clipsTrack = timeline.addFlags("Clips");
    topicsTrack = timeline.addFlags("Topics");
    questionsTrack = timeline.addFlags("Questions");
    
    timeLineActive = true;
    string previousTopic = "";
    string currentTopic = "";
    
    for(int i=0; i < actItems.size(); i++){
        ActTimeItem& item = actItems[i];
        
        if(item.type == Clip){
            currentTopic = topicMap[item.key];
            
            clipsTrack->addFlagAtTime(item.key, item.startTime * 1000);
            
            if(currentTopic != previousTopic){
                topicsTrack->addFlagAtTime(currentTopic, item.startTime * 1000);
            }
            
            previousTopic = currentTopic;
            
        }
        else if(item.type == VS){
            visualSystemsTrack->addFlagAtTime("start:" + item.key, item.startTime * 1000);
            visualSystemsTrack->addFlagAtTime("end:" + item.key, item.endTime * 1000);
        }
        else if (item.type == Gap){
            //nothing for now
        }
        else if(item.type == Question){
            questionsTrack->addFlagAtTime(item.key, item.startTime*1000);
        }
    }

    ofAddListener(timeline.events().bangFired, this, &CloudsAct::timelineEventFired);
}

void CloudsAct::timelineEventFired(ofxTLBangEventArgs& bang){
    if(bang.track == clipsTrack){
        CloudsStoryEventArgs args(clipMap[bang.flag], "");
        ofNotifyEvent(events.clipBegan, args);
    }
    else if(bang.track == visualSystemsTrack){
        //split string on %, send VS either began or ended
        vector <string> presetId;
        presetId = ofSplitString(bang.flag, ":");
        CloudsVisualSystemEventArgs args(visualSystemsMap[presetId[1]]);
        ofNotifyEvent(events.visualSystemBegan, args);
    }
    else if(bang.track == questionsTrack){
        CloudsQuestionEventArgs args(questionsMap[bang.flag]);
        ofNotifyEvent(events.askQuestion, args);
    }
}

bool CloudsAct::clipEnded(){
    
    CloudsStoryEventArgs args(currentClip,currentTopic);
    ofNotifyEvent(events.clipEnded, args);
    
    if(currentPlayIndex<clips.size()){
        waitingForNextClip = true;
        nextClipTime = timer.getAppTimeSeconds()+args.timeUntilNextClip;
    }
    else{
        ofNotifyEvent(events.actEnded, args);
    }
}


float CloudsAct::getActDuration(){
    float totalDuration = 0;
    for(int i=0; i < clips.size(); i++){
        totalDuration += clips[i].getDuration();
    }
}


void CloudsAct::drawActDebug(){
    
    timeline.setOffset(ofVec2f(0,ofGetHeight()/3));
    timeline.draw();
}

CloudsClip& CloudsAct::getClipInAct(int index){
    return clips[index];
}

ActTimeItem& CloudsAct::getItemForClip(CloudsClip& clip){
    if(clipMap.find(clip.getLinkName()) == clipMap.end()){
        ofLogError() << "Couldn't find Act Item for cilp " << clip.getLinkName();
        return dummy;
    }
    return clipItems[clip.getLinkName()];
}

void CloudsAct::addClipToAct(CloudsClip clip, string topic, float startTime){
    clips.push_back(clip);
    clipMap[clip.getLinkName()] = clip;
    topicMap[clip.getLinkName()] = topic;
    
    cout<<"added " <<clip.getLinkName()<< " to clip map "<<endl;
    ActTimeItem item;
    
    item.type = Clip;
    item.key =clip.getLinkName();
    item.startTime = startTime;
    item.endTime = startTime+clip.getDuration();
    duration = MAX(item.endTime, duration);
    
    actItems.push_back(item);
    clipItems[clip.getLinkName()] = item;
}

void CloudsAct::addVisualSystem(CloudsVisualSystemPreset preset, float startTime, float duration){
    visualSystems.push_back(preset);
    visualSystemsMap[preset.getID()] = preset;
    
    cout<<"added " <<preset.getID()<< " to VS map "<<endl;
    ActTimeItem item;
    // start the visual system halfway through the clip
    float vsStartTime  = startTime;
    item.type = VS;
    item.key =preset.getID();
    item.startTime = vsStartTime;
    
    item.endTime = vsStartTime + duration;
    
    duration = MAX(item.endTime, duration);
    
    actItems.push_back(item);
    visualSystemItems[preset.getID()] = item;
    
}

void CloudsAct::addGapForVisualSystem(float startTime){
    ActTimeItem item;
    item.type = Gap;
    item.startTime = startTime;
    item.endTime = startTime + visualSystemDuration;
    
    duration = MAX(item.endTime, duration);
    
    actItems.push_back(item);
    
}
void CloudsAct::addQuestionToAct(CloudsClip clip, float startTime, float duration){
    ActTimeItem item;
    item.type = Question;
    item.key = clip.startingQuestion;
    item.startTime = startTime;
    item.endTime = startTime + duration;

    questionsMap[clip.startingQuestion] = clip;
    //TODO: Check if you need to update duratio here. I dont think you do.
//    duration = MAX(item.endTIme, duration);
    actItems.push_back(item);
}

CloudsClip& CloudsAct :: getClipForQuestion(string question){
    if(questionsMap.find(question) == questionsMap.end()){
        ofLogError() << "Couldn't find Clip Item with Starting Question " << question;
        return dummyClip;
    }
    return questionsMap[question];
}

vector<CloudsClip>& CloudsAct::getAllClips(){
    return clips;
}

string CloudsAct::getTopicForClip(CloudsClip& clip){
    return topicMap[ clip.getLinkName() ];
}

vector<string>& CloudsAct::getAllTopics(){
    return topicHistory;
}

//void CloudsAct::addTopicToHistory(string topic){
//    topicHistory.push_back(topic);
//}

void CloudsAct::setTopicForClip(string topic, string clipName)
{
    topicMap[clipName] = topic;
}

void CloudsAct::clearAct(){
    clips.clear();
    topicHistory.clear();
    clipMap.clear();
    visualSystemsMap.clear();
    topicMap.clear();
    actItems.clear();
    timeline.reset();
    duration = 0;
}


CloudsEvents& CloudsAct::getEvents(){
	return events;
}