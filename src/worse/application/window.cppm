module;

#include "worse/core/macro.hpp"

#include "GLFW/glfw3.h"

export module worse.application.window;
import worse.core.basic_type;

namespace worse::application
{

    export enum class EWindowMode {
        Fullscreen,
        WindowedFullscreen,
        Windowed,

        Num
    };

    export struct WindowDefinition
    {
        f32 xDesiredPositionOnScreen = 0;
        f32 yDesiredPositionOnScreen = 0;
        f32 widthDesiredOnScreen     = 900;
        f32 heightDesiredOnscreen    = 600;
        bool bSupportTransparency    = false;
        bool bIsResizable            = true;
        bool bHasTabAndBorder        = true;
        bool bIsAlwaysOnTop          = false;
        bool bAcceptInput            = false;
    };

    export class Window
    {
    public:
        Window()
            : mpHandle{nullptr}
            , mWindowMode{EWindowMode::Windowed}
            , mbIsVisable{false}
            , mbIsFirstTimeVisiable{true}
        {
        }

        virtual ~Window()
        {
        }

        void initialize(WindowDefinition const& definition, bool const bShowImmediately) noexcept
        {
            WE_MAYBE_UNUSED bool const bGLFWInitialized = glfwInit() == GLFW_TRUE;
            WE_ASSERT_MSG(bGLFWInitialized, "Failed to initialize GLFW");

            mWindowDefinition     = definition;
            mbIsFirstTimeVisiable = bShowImmediately;
            mXPosition            = static_cast<i32>(mWindowDefinition.xDesiredPositionOnScreen);
            mYPosition            = static_cast<i32>(mWindowDefinition.yDesiredPositionOnScreen);
            mWidth                = static_cast<i32>(mWindowDefinition.widthDesiredOnScreen);
            mHeight               = static_cast<i32>(mWindowDefinition.heightDesiredOnscreen);

            glfwDefaultWindowHints();

            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, definition.bSupportTransparency ? GLFW_TRUE : GLFW_FALSE);
            glfwWindowHint(GLFW_RESIZABLE, definition.bIsResizable ? GLFW_TRUE : GLFW_FALSE);
            glfwWindowHint(GLFW_DECORATED, definition.bHasTabAndBorder ? GLFW_TRUE : GLFW_FALSE);
            glfwWindowHint(GLFW_FLOATING, definition.bIsAlwaysOnTop ? GLFW_TRUE : GLFW_FALSE);
            glfwWindowHint(GLFW_VISIBLE, mbIsFirstTimeVisiable ? GLFW_TRUE : GLFW_FALSE);

            constexpr char const* windowName = "worse";

            mpHandle = glfwCreateWindow(mWidth, mHeight, windowName, nullptr, nullptr);

            WE_ASSERT_MSG(mpHandle != nullptr, "Failed to create window");

            glfwSetWindowPos(mpHandle, mXPosition, mYPosition);
        }

        void destory() noexcept
        {
            glfwDestroyWindow(mpHandle);
            glfwTerminate();
        }

        void moveWindowTo(i32 x, i32 y) noexcept
        {
            mXPosition = x;
            mYPosition = y;
            glfwSetWindowPos(mpHandle, mXPosition, mYPosition);
        }

        void adjustWindowSize(i32 width, i32 height) noexcept
        {
            mWidth  = width;
            mHeight = height;

            glfwSetWindowSize(mpHandle, mWidth, mHeight);
        }

        void reshapeWindow(i32 x, i32 y, i32 width, i32 height) noexcept
        {
            mXPosition = x;
            mYPosition = y;
            mWidth     = width;
            mHeight    = height;

            glfwSetWindowPos(mpHandle, mXPosition, mYPosition);
            glfwSetWindowSize(mpHandle, mWidth, mHeight);
        }

        void Show() noexcept
        {
            if (!mbIsVisable)
            {
                mbIsVisable = true;

                glfwShowWindow(mpHandle);
            }
        }

        void Hide() noexcept
        {
            if (mbIsVisable)
            {
                mbIsVisable = false;

                glfwHideWindow(mpHandle);
            }
        }

        void Poll()
        {
            glfwPollEvents();
        }

    private:
        GLFWwindow* mpHandle;

        i32 mXPosition;
        i32 mYPosition;
        i32 mWidth;
        i32 mHeight;

        EWindowMode mWindowMode;

        /** Whether the window is currently visiable */
        bool mbIsVisable;
        bool mbIsFirstTimeVisiable;

        WindowDefinition mWindowDefinition;
    };

}; // namespace worse::application