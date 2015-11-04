class EarOSCServer : public OscMessageListener, public ChangeBroadcaster
{
public:
    String key, scale;
    float rmsValue, spectralFlatnessValue, spectralCentroidValue;
    
    EarOSCServer (int localPort)
    {
        oscServer = new OscServer (this);
        // listen on port 
//        oscServer->setLocalPortNumber (localPort);
        oscServer->setLocalPortNumber (12347);
        // start listening
        oscServer->listen ();
//        // set remote hostname (we use our self)
//        oscServer->setRemoteHostname ("127.0.0.1");
//        // set remote port (send to listening port)
//        oscServer->setRemotePortNumber (8000);
        
        key = "";
        scale = "";
    }
    ~EarOSCServer ()
    {
        oscServer = nullptr;
    }
    // called by osc server on incomming osc messsages
    void handleOscMessage (osc::ReceivedPacket packet)
    {
        // just dump the message
//        std::cout << packet << std::endl;
        
        osc::ReceivedMessage m(packet);

        if( strcmp( m.AddressPattern(), "/earData" ) == 0 ){
            // example #1 -- argument stream interface
            osc::ReceivedMessageArgumentStream args = m.ArgumentStream();
            
//            const char *currentKey, *currentScale;
//            float rmsValue, spectralFlatnessValue, spectralCentroidValue;
            
            const char *key, *scale;

            args >> key >> scale >> rmsValue >> spectralFlatnessValue >> spectralCentroidValue >> osc::EndMessage;
            
            this->key = String(key);
            this->scale = String(scale);
        
            sendChangeMessage();
            
        }

    }
    // send out a osc message
    void sendOscMessage ()
    {
//        static const int bufferSize = 1024;
//        String address = "/test/";
//        char buffer[bufferSize];
//        osc::OutboundPacketStream oscMessage (buffer, bufferSize);
//        oscMessage << osc::BeginMessage (address.toRawUTF8 ()) << 0.0 << osc::EndMessage;
//        oscServer->sendMessage (oscMessage);
    }
private:
    ScopedPointer<OscServer> oscServer;
};