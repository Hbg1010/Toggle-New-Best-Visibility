#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
using namespace geode::prelude;

void toggleLayerDetails(bool mode);
bool isCurrentlyVisible;

class $modify(bestFinder, PlayLayer) {
    struct Fields {
        CCNode* NewBestNode;
        CCNode* FadeLayer;
        bool saveAcrossAttempts;
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        // if people want this to be editable between attempts I could do that later...
        Mod* ModPtr = Mod::get();
        m_fields->saveAcrossAttempts = Mod::get()->getSettingValue<bool>("SaveAcrossAttempts");

        if (!ModPtr->getSettingValue<bool>("SaveAcrossLevels")) {
            isCurrentlyVisible = !ModPtr->getSettingValue<bool>("HideByDefault");
        }

        return true;
    }

    void showNewBest(bool p0, int p1, int p2, bool p3, bool p4, bool p5) {
        PlayLayer::showNewBest(p0, p1, p2, p3, p4, p5);

        auto children = this->getChildren();
        m_fields->NewBestNode = nullptr;

        for (int i = this->getChildrenCount() - 1; i >= 0; i--) {
            auto child = static_cast<CCNode*>(children->objectAtIndex(i));
            // bar for bar taken from Ery, 
            if (!child || child == this->m_uiLayer) continue; // skip UILayer
			if (child->getZOrder() != 100) continue;
			if (child->getChildrenCount() < 2) continue;
            child->setUserObject("new-best-node"_spr, CCBool::create(true)); // set the user object to identify the node
            m_fields->NewBestNode = child;
			break;
        }

        if (Mod::get()->getSettingValue<bool>("HideWithoutPause") && m_fields->NewBestNode != nullptr) {
            toggleLayerDetails(isCurrentlyVisible);
        }
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        if (!isCurrentlyVisible && !m_fields->saveAcrossAttempts) isCurrentlyVisible = true;
        if (isNewBest()) {
            m_fields->NewBestNode = nullptr;
            m_fields->FadeLayer = nullptr;
        }
    }
    
    void onExit() {
        if (Mod::get()->getSettingValue<bool>("SaveAcrossLevels")) {
            Mod::get()->setSavedValue<bool>("visibleBest", isCurrentlyVisible);
        }
        PlayLayer::onExit();
    }

    bool isNewBest() {
        return m_fields->NewBestNode != nullptr;
    }

    CCNode* getNewBestNode() {
        return m_fields->NewBestNode;
    }

    // I don't love this code, however, it only generates the fade layer after a bit
    // and it cannot be indexed starting from 0 for whatever reason.
    CCNode* getFadeLayer() {
        if (m_fields->FadeLayer == nullptr) {
            auto children = this->getChildren();
            for (int i = this->getChildrenCount() - 1; i >= 0; i--) {
                auto child = static_cast<CCNode*>(children->objectAtIndex(i));
                if (!child || child == this->m_uiLayer) continue; // skip UILayer
                if (child->getZOrder() != 99) continue;
                if (child->getChildrenCount() > 0) continue;
                m_fields->FadeLayer = child;
                break;
            }
        }

        return m_fields->FadeLayer;
    }
};

// hides new best
void toggleLayerDetails(bool mode) {
    // apply changes to play layer
    bestFinder* pl = reinterpret_cast<bestFinder*>(bestFinder::get());
    if (pl && pl->isNewBest()) {    
        if (auto BN = pl->getNewBestNode()) {
            if (BN != nullptr) BN->setVisible(mode);
        } else {
            return;
        }

        if (pl->m_level->m_stars == 0) return;

        if (auto currency = pl->getChildByType<CurrencyRewardLayer>(0)) {
            if (currency != nullptr) {
                currency->setVisible(mode);

                // fade layer only displays if the level is rated
                if (auto FadeLay = pl->getFadeLayer()) {
                    if (FadeLay != nullptr) FadeLay->setVisible(mode);        
                }
            };
        }


    }
}

class $modify(bestDisabler, PauseLayer) {

    void customSetup() {
        PauseLayer::customSetup();
        auto modPtr = Mod::get();
        if (!modPtr->getSettingValue<bool>("enable")) return;

        // checks if the playlayer exists + if there's a new best (bc of a setting)
        if (bestFinder* pl = reinterpret_cast<bestFinder*>(bestFinder::get())) {
            if (modPtr->getSettingValue<bool>("showOnlyOnBest") && !pl->isNewBest()) return;
        } else {
            return;
        }
        
        // create the button
        auto spr = CCSprite::createWithSpriteFrameName("GJ_newBest_001.png");
        spr->setScale(0.5f);
        auto hideBtn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(bestDisabler::onHideBtn));
        enableSprite(hideBtn, isCurrentlyVisible);
        toggleLayerDetails(isCurrentlyVisible);
        hideBtn->setPositionX(this->getChildByID("bottom-button-menu")->getContentWidth()/2);

        #ifdef GEODE_IS_MOBILE
        hideBtn->setPositionY(hideBtn->getScaledContentHeight()*2/3);
        #else
        hideBtn->setPositionY(hideBtn->getScaledContentHeight()/2);
        #endif
        this->getChildByID("bottom-button-menu")->addChild(hideBtn);
        
        hideBtn->setID("Hide_Best_Btn"_spr);
    }
 
    void onHideBtn(CCObject* sender) {
        isCurrentlyVisible = !isCurrentlyVisible;
        enableSprite(sender, isCurrentlyVisible);
        toggleLayerDetails(isCurrentlyVisible);
    } 

    // sets the enabled sprite color
    void enableSprite(CCObject* node, bool enable) {
        //TODO Find a better spot to put this
        if (node == nullptr) return;
        
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

$on_mod(Loaded) {
    if (Mod::get()->getSettingValue<bool>("SaveAcrossLevels")) {
        isCurrentlyVisible = Mod::get()->getSavedValue<bool>("visibleBest", true);
    } else {
        isCurrentlyVisible = !Mod::get()->getSettingValue<bool>("HideByDefault");
    }
}