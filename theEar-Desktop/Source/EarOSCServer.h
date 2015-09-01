class EarOSCServer : public OscMessageListener
{
public:
    EarOSCServer (int localPort)
    {
        oscServer = new OscServer (this);
        // listen on port 
        oscServer->setLocalPortNumber (localPort);
        // start listening
        oscServer->listen ();
//        // set remote hostname (we use our self)
//        oscServer->setRemoteHostname ("127.0.0.1");
//        // set remote port (send to listening port)
//        oscServer->setRemotePortNumber (8000);
    }
    ~EarOSCServer ()
    {
        oscServer = nullptr;
    }
    // called by osc server on incomming osc messsages
    void handleOscMessage (osc::ReceivedPacket packet)
    {
        // just dump the message
        std::cout << packet << std::endl;
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