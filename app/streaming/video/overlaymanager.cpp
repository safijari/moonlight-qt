#include "overlaymanager.h"
#include "path.h"
#include <iostream>

using namespace Overlay;

OverlayManager::OverlayManager() :
    m_Renderer(nullptr),
    m_FontData(Path::readDataFile("ModeSeven.ttf"))
{
    memset(m_Overlays, 0, sizeof(m_Overlays));

    m_Overlays[OverlayType::OverlayDebug].color = {0xD0, 0xD0, 0x00, 0xFF};
    m_Overlays[OverlayType::OverlayDebug].fontSize = 20;

    m_Overlays[OverlayType::OverlayStatusUpdate].color = {0xCC, 0x00, 0x00, 0xFF};
    m_Overlays[OverlayType::OverlayStatusUpdate].fontSize = 36;

    m_Overlays[OverlayType::OverlayGraph].color = {0xCC, 0x00, 0x00, 0xFF};

    // While TTF will usually not be initialized here, it is valid for that not to
    // be the case, since Session destruction is deferred and could overlap with
    // the lifetime of a new Session object.
    //SDL_assert(TTF_WasInit() == 0);

    if (TTF_Init() != 0) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "TTF_Init() failed: %s",
                    TTF_GetError());
        return;
    }
}

OverlayManager::~OverlayManager()
{
    for (int i = 0; i < OverlayType::OverlayMax; i++) {
        if (m_Overlays[i].surface != nullptr) {
            SDL_FreeSurface(m_Overlays[i].surface);
        }
        if (m_Overlays[i].font != nullptr) {
            TTF_CloseFont(m_Overlays[i].font);
        }
    }

    TTF_Quit();

    // For similar reasons to the comment in the constructor, this will usually,
    // but not always, deinitialize TTF. In the cases where Session objects overlap
    // in lifetime, there may be an additional reference on TTF for the new Session
    // that means it will not be cleaned up here.
    //SDL_assert(TTF_WasInit() == 0);
}

bool OverlayManager::isOverlayEnabled(OverlayType type)
{
    return m_Overlays[type].enabled;
}

char* OverlayManager::getOverlayText(OverlayType type)
{
    return m_Overlays[type].text;
}

int OverlayManager::getOverlayFontSize(OverlayType type)
{
    return m_Overlays[type].fontSize;
}

SDL_Surface* OverlayManager::getUpdatedOverlaySurface(OverlayType type)
{
    // If a new surface is available, return it. If not, return nullptr.
    // Caller must free the surface on success.
    return (SDL_Surface*)SDL_AtomicSetPtr((void**)&m_Overlays[type].surface, nullptr);
}

void OverlayManager::setOverlayTextUpdated(OverlayType type)
{
    // Only update the overlay state if it's enabled. If it's not enabled,
    // the renderer has already been notified by setOverlayState().
    if (m_Overlays[type].enabled) {
        notifyOverlayUpdated(type);
    }
}

void OverlayManager::setOverlayState(OverlayType type, bool enabled)
{
    bool stateChanged = m_Overlays[type].enabled != enabled;

    m_Overlays[type].enabled = enabled;

    if (stateChanged) {
        if (!enabled) {
            // Set the text to empty string on disable
            m_Overlays[type].text[0] = 0;
        }

        notifyOverlayUpdated(type);
    }
}

SDL_Color OverlayManager::getOverlayColor(OverlayType type)
{
    return m_Overlays[type].color;
}

void OverlayManager::setOverlayRenderer(IOverlayRenderer* renderer)
{
    m_Renderer = renderer;
}

void OverlayManager::notifyOverlayUpdated(OverlayType type)
{
    if (m_Renderer == nullptr) {
        return;
    }

    SDL_Surface* oldSurface = (SDL_Surface*)SDL_AtomicSetPtr((void**)&m_Overlays[type].surface, nullptr);
    // Free the old surface
    if (oldSurface != nullptr) {
        SDL_FreeSurface(oldSurface);
    }


    if (type != OverlayType::OverlayGraph) {
        // Construct the required font to render the overlay
        if (m_Overlays[type].font == nullptr) {
            if (m_FontData.isEmpty()) {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                            "SDL overlay font failed to load");
                return;
            }

            // m_FontData must stay around until the font is closed
            m_Overlays[type].font = TTF_OpenFontRW(SDL_RWFromConstMem(m_FontData.constData(), m_FontData.size()),
                                                1,
                                                m_Overlays[type].fontSize);
            if (m_Overlays[type].font == nullptr) {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                            "TTF_OpenFont() failed: %s",
                            TTF_GetError());

                // Can't proceed without a font
                return;
            }
        }

        if (m_Overlays[type].enabled) {
            // The _Wrapped variant is required for line breaks to work
            SDL_Surface* surface = TTF_RenderText_Blended_Wrapped(m_Overlays[type].font,
                                                                m_Overlays[type].text,
                                                                m_Overlays[type].color,
                                                                1024);

            // auto rdr = SDL_CreateSoftwareRenderer(surface);
            // std::cout << "color " << SDL_SetRenderDrawColor(rdr, 242, 0, 0, 255) << std::endl;
            // std::cout << "line " << SDL_RenderDrawLine(rdr, 0, 0, 1000, 1000) << std::endl;
            // SDL_RenderPresent(rdr);
            // SDL_DestroyRenderer(rdr);
            SDL_AtomicSetPtr((void**)&m_Overlays[type].surface, surface);
        }
    } else {
        if (m_Overlays[type].enabled) {
          // SDL_Surface* surface = TTF_RenderText_Blended_Wrapped(m_Overlays[OverlayType::OverlayDebug].font,
          //                                                       "llllloooooooooooooooooooooooooooooooooooooolllllllll",
          //                                                       m_Overlays[type].color,
          //                                                       1024);

            auto surface = SDL_CreateRGBSurfaceWithFormat(0, 500, 500, 32, SDL_PIXELFORMAT_BGRA32);
            auto rdr = SDL_CreateSoftwareRenderer(surface);
            std::cout << "color " << SDL_SetRenderDrawColor(rdr, 242, 0, 0, 255) << std::endl;
            std::cout << "line " << SDL_RenderDrawLine(rdr, 0, 0, 500, 500) << std::endl;
            SDL_RenderPresent(rdr);
            SDL_DestroyRenderer(rdr);
            SDL_AtomicSetPtr((void**)&m_Overlays[type].surface, surface);
            // auto rdr = SDL_CreateSoftwareRenderer(surface);
            // std::cout << "color " << SDL_SetRenderDrawColor(rdr, 242, 0, 0, 255) << std::endl;
            // std::cout << "line " << SDL_RenderDrawLine(rdr, 0, 0, 500, 500) << std::endl;
            // SDL_RenderPresent(rdr);
            // SDL_DestroyRenderer(rdr);
            // SDL_AtomicSetPtr((void**)&m_Overlays[type].surface, surface);
        }

    }

    // Notify the rdr
    m_Renderer->notifyOverlayUpdated(type);
}
