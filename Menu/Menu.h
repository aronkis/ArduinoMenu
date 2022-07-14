#include <iostream>

// #include<Arduino.h> PROGMEM --- 


class MenuNode{
    private:
        const char* menuText;
        int MenuFID;
        uint8_t menuLayer;
    public:
        MenuNode(){ menuText = "", MenuFID = 0, menuLayer = 0;}
        MenuNode(const char* const fText, const int fFID, const uint8_t fLayer) {menuText = fText; MenuFID = fFID ; menuLayer = fLayer;}
        ~MenuNode() {}
        void getInfo(const char*& fInfo) { fInfo = menuText; }
        void setLayer(const uint8_t fLayer) { menuLayer = fLayer; }
        int getFID() { return MenuFID; }
        uint8_t getLayer(){ return menuLayer; }
};

template <int maxElements> class Menu{
    
    private:
        MenuNode menuNodes[maxElements];
        MenuNode* menuTopNode;
        int menuIndex, nrUsedElements;
        
    public:
    Menu() { menuIndex = 0; menuTopNode = 0; nrUsedElements = 0;}
    ~Menu(){}

    void addNode(int fLayer, const char* const fText, const int fFID){
        if (menuIndex < maxElements){
            menuNodes[menuIndex] = MenuNode(fText, fFID, fLayer);
            menuIndex++;
            nrUsedElements++;
        }
    }
    
    void printMenu(){
        std::cout << "Menu: \n";
        for (int i = 0; i < nrUsedElements; i++){
            for (int j = 0; j < menuNodes[i].getLayer(); j++)
                std::cout << "\t";
            // with return?
            const char* info;
            menuNodes[i].getInfo(info);
            /*
            String info_s;
            MBHelper::stringFromPgm(info, info_s); REVIEW
            Serial.print(info_s);
            Serial.print(F(", "));
            Serial.println(m_nodes[i].getFID());
            */
            std::cout << info << ", " << menuNodes[i].getFID() << "\n";
        }
    }
    
    int buildMenu(const char* &fInfo) {
        menuIndex = 0;
        menuNodes[menuIndex].getInfo(fInfo);
        return menuNodes[menuIndex].getFID();
  }
};
