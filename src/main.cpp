#include <Geode/Geode.hpp>
using namespace geode::prelude;

#include <Geode/modify/PlayLayer.hpp>
class $modify(bestFinder, PlayLayer) {
    void showNewBest(bool p0, int p1, int p2, bool p3, bool p4, bool p5) {
        PlayLayer::showNewBest(p0, p1, p2, p3, p4, p5);

        auto children = this->getChildren();
        CCNode* lastChild = nullptr;

        for (int i = this->getChildrenCount() - 1; i >= 0; i--) {
            auto child = static_cast<CCNode*>(children->objectAtIndex(i));
            // bar for bar taken from Ery, 
            if (!child || child == this->m_uiLayer) continue; // skip UILayer
			if (child->getZOrder() != 100) continue;
			if (child->getChildrenCount() < 2) continue;
            child->setID("NewBestNode");
			break;
        }
    }
};

#include <Geode/modify/PauseLayer.hpp>
class $modify(bestDisabler, PauseLayer) {

    struct Fields {
        bool onOff;
    };

    void customSetup() {
        PauseLayer::customSetup();
        auto modPtr = Mod::get();
        if (!modPtr->getSettingValue<bool>("enable")) return;

        auto fields = m_fields.self(); // gets the ptr

        if (modPtr->getSettingValue<bool>("showOnlyOnBest")) {
            if (auto pl = PlayLayer::get()) {
                if (auto bestNode = pl->getChildByID("NewBestNode")) {
                    if (modPtr->getSettingValue<bool>("SaveAcrossLevels")) {
                        fields->onOff = !modPtr->getSavedValue<bool>("visibleBest", true);
                    } else {
                        fields->onOff = !bestNode->isVisible();
                    }
                } else {
                    return;
                }
            }
        } else {
            if (modPtr->getSettingValue<bool>("SaveAcrossLevels")) {
                fields->onOff = !modPtr->getSavedValue<bool>("visibleBest", true);
            } else {
                fields->onOff = true;
            }
        }

        // create the button
        auto spr = CCSprite::createWithSpriteFrameName("GJ_newBest_001.png");
        spr->setScale(0.5f);
        auto hideBtn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(bestDisabler::onHideBtn));
        onHideBtn(hideBtn); // this is used to set the menu to the correct state
        hideBtn->setPositionX(this->getChildByID("bottom-button-menu")->getContentWidth()/2);
        this->getChildByID("bottom-button-menu")->addChild(hideBtn);

        hideBtn->setID("Hide_Best_Btn"_spr);
    }

    void toggleLayerDetails(bool mode) {
        // apply changes to play layer
        if (auto pl = PlayLayer::get()) {    
            if (auto bestNode = pl->getChildByID("NewBestNode")) {
                bestNode->setVisible(mode);
            }

            if (auto currency = pl->getChildByType<CurrencyRewardLayer>(0)) {
                currency->setVisible(mode);
            }
        }
    }
 
    void onHideBtn(CCObject* sender) {
        bool& onOff = m_fields->onOff;
        onOff = !onOff;
        Mod::get()->setSavedValue<bool>("visibleBest", onOff);
        enableSprite(sender, onOff);
        toggleLayerDetails(onOff);
    } 

    // sets the enabled sprite color
    void enableSprite(CCObject* node, bool enable) {
        //TODO Find a better spot to put this
        const ccColor3B greyScale = {.r = 90, .g = 90, .b = 90};
        const ccColor3B color = {.r = 255, .g = 255, .b = 255};

        if (auto btn = typeinfo_cast<CCMenuItemSpriteExtra*>(node)) {
            auto spr = typeinfo_cast<CCRGBAProtocol*>(btn->getNormalImage());
                spr->setCascadeColorEnabled(true);
                spr->setCascadeOpacityEnabled(true);
                spr->setColor(enable ? color : greyScale);
                spr->setOpacity(enable ? 255 : 200);
        }
    }

    // shows the gui on resume if the setting is enabled
    void onResume(CCObject* sender) {
        PauseLayer::onResume(sender);
        if (Mod::get()->getSettingValue<bool>("showOnResume")) toggleLayerDetails(true);
    }
};