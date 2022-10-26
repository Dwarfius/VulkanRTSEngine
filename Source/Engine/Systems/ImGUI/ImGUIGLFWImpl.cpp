#include "Precomp.h"
#include "ImGUIGLFWImpl.h"

#include "Game.h"
#include "Input.h"

ImGUIGLFWImpl::ImGUIGLFWImpl(Game& aGame)
	: myGame(aGame)
{
}

void ImGUIGLFWImpl::Init(GLFWwindow& aWindow)
{
    // Setup back-end capabilities flags
    ImGuiIO& io = ImGui::GetIO();
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;         // We can honor GetMouseCursor() values (optional)
    io.BackendPlatformName = "ImGUIGLFWImpl";

    // Keyboard mapping. ImGui will use those indices to peek into the io.KeysDown[] array.
    auto Remap = [&](int anImGUIkey, Input::Keys anEngineKey) {
        io.KeyMap[anImGUIkey] = static_cast<int>(anEngineKey);
    };
    Remap(ImGuiKey_Tab, Input::Keys::Tab);
    Remap(ImGuiKey_LeftArrow, Input::Keys::Left);
    Remap(ImGuiKey_RightArrow, Input::Keys::Right);
    Remap(ImGuiKey_UpArrow, Input::Keys::Up);
    Remap(ImGuiKey_DownArrow, Input::Keys::Down);
    Remap(ImGuiKey_PageUp, Input::Keys::PageUp);
    Remap(ImGuiKey_PageDown, Input::Keys::PageDown);
    Remap(ImGuiKey_Home, Input::Keys::Home);
    Remap(ImGuiKey_End, Input::Keys::End);
    Remap(ImGuiKey_Insert, Input::Keys::Insert);
    Remap(ImGuiKey_Delete, Input::Keys::Delete);
    Remap(ImGuiKey_Backspace, Input::Keys::Backspace);
    Remap(ImGuiKey_Space, Input::Keys::Space);
    Remap(ImGuiKey_Enter, Input::Keys::Enter);
    Remap(ImGuiKey_Escape, Input::Keys::Escape);
    Remap(ImGuiKey_KeypadEnter, Input::Keys::KeyPadEnter);
    Remap(ImGuiKey_A, Input::Keys::A);
    Remap(ImGuiKey_C, Input::Keys::C);
    Remap(ImGuiKey_V, Input::Keys::V);
    Remap(ImGuiKey_X, Input::Keys::X);
    Remap(ImGuiKey_Y, Input::Keys::Y);
    Remap(ImGuiKey_Z, Input::Keys::Z);

    io.SetClipboardTextFn = [](void* user_data, const char* text) {
        glfwSetClipboardString((GLFWwindow*)user_data, text);
    };
    io.GetClipboardTextFn = [](void* user_data) {
        return glfwGetClipboardString((GLFWwindow*)user_data);
    };
    io.ClipboardUserData = &aWindow;
#if defined(_WIN32)
    ImGui::GetMainViewport()->PlatformHandleRaw = (void*)glfwGetWin32Window(&aWindow);
#endif

    // Create mouse cursors
    // (By design, on X11 cursors are user configurable and some cursors may be missing. When a cursor doesn't exist,
    // GLFW will emit an error which will often be printed by the app, so we temporarily disable error reporting.
    // Missing cursors will return NULL and our _UpdateMouseCursor() function will use the Arrow cursor instead.)
    GLFWerrorfun prev_error_callback = glfwSetErrorCallback(NULL);
    static_assert(kCursorCount == ImGuiMouseCursor_COUNT, "Update kCursorCount!");
    std::memset(myCursors, 0, kCursorCount * sizeof(GLFWcursor*));
    myCursors[ImGuiMouseCursor_Arrow] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    myCursors[ImGuiMouseCursor_TextInput] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
    myCursors[ImGuiMouseCursor_ResizeNS] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
    myCursors[ImGuiMouseCursor_ResizeEW] = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
    myCursors[ImGuiMouseCursor_Hand] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
#if GLFW_HAS_NEW_CURSORS
    myCursors[ImGuiMouseCursor_ResizeAll] = glfwCreateStandardCursor(GLFW_RESIZE_ALL_CURSOR);
    myCursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_RESIZE_NESW_CURSOR);
    myCursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_RESIZE_NWSE_CURSOR);
    myCursors[ImGuiMouseCursor_NotAllowed] = glfwCreateStandardCursor(GLFW_NOT_ALLOWED_CURSOR);
#else
    myCursors[ImGuiMouseCursor_ResizeAll] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    myCursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    myCursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    myCursors[ImGuiMouseCursor_NotAllowed] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
#endif
    glfwSetErrorCallback(prev_error_callback);
}

void ImGUIGLFWImpl::Shutdown()
{
    for(GLFWcursor* cursor : myCursors)
    {
        glfwDestroyCursor(cursor);
    }
}

void ImGUIGLFWImpl::NewFrame(float aDeltaTime)
{
    ImGuiIO& io = ImGui::GetIO();
    ASSERT_STR(io.Fonts->IsBuilt(), "Font atlas not built! It is generally built by the renderer back-end. Missing call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL3_NewFrame().");

    // Setup display size (every frame to accommodate for window resizing)
    GLFWwindow* window = myGame.GetWindow();
    int w, h;
    int display_w, display_h;
    glfwGetWindowSize(window, &w, &h);
    glfwGetFramebufferSize(window, &display_w, &display_h);
    io.DisplaySize = ImVec2((float)w, (float)h);
    if (w > 0 && h > 0)
    {
        io.DisplayFramebufferScale = ImVec2((float)display_w / w, (float)display_h / h);
    }

    // Setup time step
    io.DeltaTime = aDeltaTime;

    UpdateInput();
    UpdateMouseCursor();
}

void ImGUIGLFWImpl::UpdateInput() const
{
    ImGuiIO& io = ImGui::GetIO();
    GLFWwindow* window = myGame.GetWindow();

    // update keyboard...
    for (uint8_t keyInd = 0; keyInd < static_cast<uint8_t>(Input::Keys::Count); keyInd++)
    {
        io.KeysDown[keyInd] = Input::GetKey(static_cast<Input::Keys>(keyInd));
    }
    io.KeyCtrl = Input::GetKey(Input::Keys::Control);
    io.KeyShift = Input::GetKey(Input::Keys::Shift);
    io.KeyAlt = Input::GetKey(Input::Keys::Alt);
    io.KeySuper = Input::GetKey(Input::Keys::Super);

    // ..add input chars...
    const uint32_t* inputChars = Input::GetInputChars();
    uint32_t inputChar;
    while ((inputChar = *inputChars++))
    {
        io.AddInputCharacter(inputChar);
    }

    // ...mouse buttons...
    for (int i = 0; i < IM_ARRAYSIZE(io.MouseDown); i++)
    {
        io.MouseDown[i] = Input::GetMouseBtn(i);
    }
    
    // ...mouse scroll...
    io.MouseWheel += Input::GetMouseWheelDelta();

    // ...mouse position
    const ImVec2 mouse_pos_backup = io.MousePos;
    io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
    const bool focused = myGame.IsWindowInFocus();
    if (focused)
    {
        const glm::vec2& pos = Input::GetMousePos();
        io.MousePos = ImVec2(pos.x, pos.y);
    }
}

void ImGUIGLFWImpl::UpdateMouseCursor() const
{
    ImGuiIO& io = ImGui::GetIO();
    GLFWwindow* window = myGame.GetWindow();
    if ((io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) 
        || glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
    {
        return;
    }

    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor)
    {
        // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    }
    else
    {
        // Show OS mouse cursor
        // FIXME-PLATFORM: Unfocused windows seems to fail changing the mouse cursor with GLFW 3.2, but 3.3 works here.
        GLFWcursor* cursor = myCursors[imgui_cursor] ? myCursors[imgui_cursor] : myCursors[ImGuiMouseCursor_Arrow];
        glfwSetCursor(window, cursor);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}