#include "engine/JackAudioBackend.hpp"
#include <iostream>
#include <cstring>

namespace beater {

JackAudioBackend::JackAudioBackend() {
}

JackAudioBackend::~JackAudioBackend() {
    shutdown();
}

bool JackAudioBackend::initialize(const std::string& clientName) {
    if (client_ != nullptr) {
        std::cerr << "JACK client already initialized\n";
        return false;
    }
    
    jack_status_t status;
    client_ = jack_client_open(clientName.c_str(), JackNullOption, &status);
    
    if (client_ == nullptr) {
        std::cerr << "Failed to create JACK client. Status: " << status << "\n";
        if (status & JackServerFailed) {
            std::cerr << "Unable to connect to JACK server. Is JACK running?\n";
        }
        return false;
    }
    
    std::cout << "JACK client '" << clientName << "' created successfully\n";
    
    // Get initial sample rate and buffer size
    sampleRate_ = jack_get_sample_rate(client_);
    bufferSize_ = jack_get_buffer_size(client_);
    
    std::cout << "Sample rate: " << sampleRate_ << " Hz\n";
    std::cout << "Buffer size: " << bufferSize_ << " frames\n";
    
    // Set callbacks
    jack_set_process_callback(client_, processCallback, this);
    jack_set_sample_rate_callback(client_, sampleRateCallback, this);
    jack_set_buffer_size_callback(client_, bufferSizeCallback, this);
    jack_set_xrun_callback(client_, xrunCallback, this);
    jack_on_shutdown(client_, shutdownCallback, this);
    
    // Create output ports
    outPortLeft_ = jack_port_register(client_, "out_L",
                                       JACK_DEFAULT_AUDIO_TYPE,
                                       JackPortIsOutput, 0);
    
    outPortRight_ = jack_port_register(client_, "out_R",
                                        JACK_DEFAULT_AUDIO_TYPE,
                                        JackPortIsOutput, 0);
    
    if (outPortLeft_ == nullptr || outPortRight_ == nullptr) {
        std::cerr << "Failed to register output ports\n";
        shutdown();
        return false;
    }
    
    // Activate client
    if (jack_activate(client_) != 0) {
        std::cerr << "Failed to activate JACK client\n";
        shutdown();
        return false;
    }
    
    std::cout << "JACK client activated\n";
    std::cout << "Output ports: " 
              << jack_port_name(outPortLeft_) << ", "
              << jack_port_name(outPortRight_) << "\n";
    
    // Auto-connect to system playback ports
    const char** ports = jack_get_ports(client_, nullptr, JACK_DEFAULT_AUDIO_TYPE,
                                        JackPortIsPhysical | JackPortIsInput);
    
    if (ports != nullptr) {
        // Connect left channel
        if (ports[0] != nullptr) {
            if (jack_connect(client_, jack_port_name(outPortLeft_), ports[0]) == 0) {
                std::cout << "Auto-connected " << jack_port_name(outPortLeft_) 
                         << " -> " << ports[0] << "\n";
            }
        }
        
        // Connect right channel
        if (ports[1] != nullptr) {
            if (jack_connect(client_, jack_port_name(outPortRight_), ports[1]) == 0) {
                std::cout << "Auto-connected " << jack_port_name(outPortRight_) 
                         << " -> " << ports[1] << "\n";
            }
        }
        
        jack_free(ports);
    } else {
        std::cout << "Note: No physical playback ports found for auto-connection\n";
    }
    
    return true;
}

void JackAudioBackend::shutdown() {
    if (client_ != nullptr) {
        std::cout << "Shutting down JACK client\n";
        jack_deactivate(client_);
        jack_client_close(client_);
        client_ = nullptr;
        outPortLeft_ = nullptr;
        outPortRight_ = nullptr;
    }
}

jack_position_t JackAudioBackend::getTransportPosition() const {
    jack_position_t pos;
    if (client_ != nullptr) {
        jack_transport_query(client_, &pos);
    } else {
        std::memset(&pos, 0, sizeof(pos));
    }
    return pos;
}

bool JackAudioBackend::isTransportRolling() const {
    if (client_ == nullptr) {
        return false;
    }
    jack_transport_state_t state = jack_transport_query(client_, nullptr);
    return state == JackTransportRolling;
}

// JACK callback implementations

int JackAudioBackend::processCallback(jack_nframes_t nframes, void* arg) {
    auto* backend = static_cast<JackAudioBackend*>(arg);
    
    // Get output buffers
    float* outL = static_cast<float*>(jack_port_get_buffer(backend->outPortLeft_, nframes));
    float* outR = static_cast<float*>(jack_port_get_buffer(backend->outPortRight_, nframes));
    
    // Clear buffers
    std::memset(outL, 0, nframes * sizeof(float));
    std::memset(outR, 0, nframes * sizeof(float));
    
    // Call audio rendering callback if set
    if (backend->audioCallback_) {
        backend->audioCallback_(nframes, outL, outR);
    }
    
    return 0;
}

int JackAudioBackend::sampleRateCallback(jack_nframes_t nframes, void* arg) {
    auto* backend = static_cast<JackAudioBackend*>(arg);
    backend->sampleRate_ = nframes;
    std::cout << "JACK sample rate changed to " << nframes << " Hz\n";
    return 0;
}

int JackAudioBackend::bufferSizeCallback(jack_nframes_t nframes, void* arg) {
    auto* backend = static_cast<JackAudioBackend*>(arg);
    backend->bufferSize_ = nframes;
    std::cout << "JACK buffer size changed to " << nframes << " frames\n";
    return 0;
}

void JackAudioBackend::shutdownCallback(void* arg) {
    auto* backend = static_cast<JackAudioBackend*>(arg);
    std::cerr << "JACK server shut down\n";
    backend->client_ = nullptr;
    backend->outPortLeft_ = nullptr;
    backend->outPortRight_ = nullptr;
}

int JackAudioBackend::xrunCallback(void* arg) {
    auto* backend = static_cast<JackAudioBackend*>(arg);
    backend->xrunCount_++;
    std::cerr << "JACK xrun detected (count: " << backend->xrunCount_ << ")\n";
    return 0;
}

} // namespace beater
