//
// Created by omer on 7.2.2020.
//

#ifndef ENGIENIGLNEW_AUDIOPLAYER_H
#define ENGIENIGLNEW_AUDIOPLAYER_H
#include <iostream>       // std::cout
#include <thread>         // std::thread

#define LenBack 65
#define LenBackLONG (15*60)+19 + 2

#include <SDL2/SDL.h>



class AudioPlayer {

public:

    AudioPlayer(const std::string &path) : path(path) {
    }

    AudioPlayer() {        SDL_Init(SDL_INIT_AUDIO);

    }


    std::string path;

    void play(const std::string &name) {
        std::string tmp = path + name;

        //if its a background
//        if(name == "background.wav")
        if(name == "longBackground.wav")
        {
            if(BackIsPlaying ) return;
            BackIsPlaying =true;
            if(SDL_LoadWAV(&tmp.at(0),
                           &wavSpec, &wavBufferB, &wavLengthB)==NULL){
                std::cout << "error loading "<< name << std::endl;
//            return;
            }

            deviceIdB = SDL_OpenAudioDevice(NULL, 0, &wavSpec, NULL, 0);

            int success = SDL_QueueAudio(deviceIdB, wavBufferB, wavLengthB);
            SDL_PauseAudioDevice(deviceIdB, 0);
        }
        else{
            SDL_AudioSpec wavSpecA;
            Uint32 wavLengthA;
            if(SDL_LoadWAV(&tmp.at(0),
                           &wavSpecA, &wavBufferA, &wavLengthA)==NULL){
                std::cout << "error loading "<< name << std::endl;
//            return;
            }

            deviceId= SDL_OpenAudioDevice(NULL, 0, &wavSpecA, NULL, 0);

            int success = SDL_QueueAudio(deviceId, wavBufferA, wavLengthA);
            SDL_PauseAudioDevice(deviceId, 0);

        }
    }

    void playBackground() {
        time(&startBackground);
        play("longBackground.wav");

    }
    SDL_AudioDeviceID deviceId;

    Uint8 *wavBufferA;

    SDL_AudioDeviceID deviceIdB;
    SDL_AudioSpec wavSpec;
    Uint32 wavLengthB;
    Uint8 *wavBufferB;

    void playHit() {
        time(&startBackground);
        play("goodMesh.wav");
    }

    void playBadHit() {
        time(&startBackground);
        play("badMesh.wav");
    }

    double backLen = LenBack-1;
    bool BackIsPlaying = false;
    time_t startBackground;
    time_t lastHit;

    void replayBackground() {


        SDL_CloseAudioDevice(deviceIdB);
        SDL_FreeWAV(wavBufferB);
        playBackground();

    }

    void checks() {
        if (difftime(time(NULL), startBackground) == LenBackLONG && !BackIsPlaying) {
            replayBackground();
        }
        if (difftime(time(NULL), lastHit) == 2) {
            SDL_CloseAudioDevice(deviceId);
            SDL_FreeWAV(wavBufferA);
        }
    }


};


#endif //ENGIENIGLNEW_AUDIOPLAYER_H
