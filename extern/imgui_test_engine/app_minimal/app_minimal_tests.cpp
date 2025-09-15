// dear imgui
// Demo Tests demonstrating the Dear ImGui Test Engine

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_test_engine/imgui_te_context.h"
#include "imgui_test_engine/imgui_capture_tool.h"

#include "imgui_reflect.hpp"

struct MyStruct {
    int MyInt = 0;
    float MyFloat = 0.0f;
};

IMGUI_REFLECT(MyStruct, MyInt, MyFloat)

void RegisterAppMinimalTests(ImGuiTestEngine* e) {
    ImGuiTest* t = nullptr;

    //-----------------------------------------------------------------
    // ## Demo Test: Hello Automation World
    //-----------------------------------------------------------------

    //t = IM_REGISTER_TEST(e, "demo_tests", "test1");
    //t->GuiFunc = [](ImGuiTestContext* ctx) {
    //    IM_UNUSED(ctx);
    //    ImGui::Begin("Test Window", nullptr, ImGuiWindowFlags_NoSavedSettings);
    //    ImGui::Text("Hello, automation world");
    //    ImGui::Button("Click Me");
    //    if (ImGui::TreeNode("Node")) {
    //        static bool b = false;
    //        ImGui::Checkbox("Checkbox", &b);
    //        ImGui::TreePop();
    //    }
    //    ImGui::End();
    //    };
    //t->TestFunc = [](ImGuiTestContext* ctx) {
    //    ctx->SetRef("Test Window");
    //    ctx->ItemClick("Click Me");
    //    ctx->ItemOpen("Node"); // Optional as ItemCheck("Node/Checkbox") can do it automatically
    //    ctx->ItemCheck("Node/Checkbox");
    //    ctx->ItemUncheck("Node/Checkbox");
    //    };

    //-----------------------------------------------------------------
    // ## Demo Test: Use variables to communicate data between GuiFunc and TestFunc
    //-----------------------------------------------------------------

    //t = IM_REGISTER_TEST(e, "demo_tests", "test2");
    //struct TestVars2 { int MyInt = 42; };
    //t->SetVarsDataType<TestVars2>();
    //t->GuiFunc = [](ImGuiTestContext* ctx) {
    //    TestVars2& vars = ctx->GetVars<TestVars2>();
    //    ImGui::Begin("Test Window", nullptr, ImGuiWindowFlags_NoSavedSettings);
    //    ImGui::SliderInt("Slider", &vars.MyInt, 0, 1000);
    //    ImGui::End();
    //    };
    //t->TestFunc = [](ImGuiTestContext* ctx) {
    //    TestVars2& vars = ctx->GetVars<TestVars2>();
    //    ctx->SetRef("Test Window");

    //    IM_CHECK_EQ(vars.MyInt, 42);
    //    ctx->ItemInputValue("Slider", 123);
    //    IM_CHECK_EQ(vars.MyInt, 123);
    //    };

    //-----------------------------------------------------------------
    // ## Open Metrics window
    //-----------------------------------------------------------------

    //t = IM_REGISTER_TEST(e, "demo_tests", "open_metrics");
    //t->TestFunc = [](ImGuiTestContext* ctx) {
    //    ctx->SetRef("Dear ImGui Demo");
    //    ctx->MenuCheck("Tools/Metrics\\/Debugger");
    //    };

    //-----------------------------------------------------------------
    // ## Capture entire Dear ImGui Demo window.
    //-----------------------------------------------------------------

    //t = IM_REGISTER_TEST(e, "demo_tests", "capture_screenshot");
    //t->TestFunc = [](ImGuiTestContext* ctx) {
    //    ctx->SetRef("Dear ImGui Demo");
    //    ctx->ItemOpen("Widgets");       // Open collapsing header
    //    ctx->ItemOpenAll("Basic");      // Open tree node and all its descendant
    //    ctx->CaptureScreenshotWindow("//Dear ImGui Demo", ImGuiCaptureFlags_StitchAll | ImGuiCaptureFlags_HideMouseCursor);
    //    };

    //t = IM_REGISTER_TEST(e, "demo_tests", "capture_video");
    //t->TestFunc = [](ImGuiTestContext* ctx) {
    //    ctx->SetRef("Dear ImGui Demo");
    //    ctx->ItemCloseAll("");
    //    ctx->MouseTeleportToPos(ctx->GetWindowByRef("")->Pos);

    //    ctx->CaptureAddWindow("//Dear ImGui Demo"); // Optional: Capture single window
    //    ctx->CaptureBeginVideo();
    //    ctx->ItemOpen("Widgets");
    //    ctx->ItemInputValue("Basic/input text", "My first video!");
    //    ctx->CaptureEndVideo();
    //    };

    //-----------------------------------------------------------------
    // ## ImGui::Reflect - reflect struct
    //-----------------------------------------------------------------

    t = IM_REGISTER_TEST(e, "imgui_reflect", "reflect struct");
    t->SetVarsDataType<MyStruct>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        MyStruct& vars = ctx->GetVars<MyStruct>();
        ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_Always);
        ImGui::Begin("Test Window", nullptr, ImGuiWindowFlags_NoSavedSettings);
        ImSettings settings;
        ImGui::Reflect::Input("MyStruct", vars, settings);
        ImGui::End();
        };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        MyStruct& vars = ctx->GetVars<MyStruct>();
        ctx->SetRef("Test Window");
        IM_CHECK_EQ(vars.MyInt, 0);
        IM_CHECK_EQ(vars.MyFloat, 0.0f);
        ctx->ItemInputValue("MyInt", 123);
        ctx->ItemInputValue("MyFloat", 0.25f);
        IM_CHECK_EQ(vars.MyInt, 123);
        IM_CHECK_EQ(vars.MyFloat, 0.25f);
        };

    //-----------------------------------------------------------------
    // ## ImGui::Reflect - slider with min/max
    //   - Test clamping and slider behavior
    //-----------------------------------------------------------------

    t = IM_REGISTER_TEST(e, "imgui_reflect", "slider min/max");
    t->SetVarsDataType<MyStruct>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        MyStruct& vars = ctx->GetVars<MyStruct>();
        ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_Always);
        ImGui::Begin("Test Window", nullptr, ImGuiWindowFlags_NoSavedSettings);
        ImSettings settings;
        settings.push<int>()
            .min(-100)
            .max(100)
            .as_slider()
            .pop()
            .push<float>()
            .min(-1.0f)
            .max(1.0f)
            .as_slider()
            .pop();
        ImGui::Reflect::Input("MyStruct", vars, settings);
        ImGui::End();
        };

    t->TestFunc = [](ImGuiTestContext* ctx) {
        MyStruct& vars = ctx->GetVars<MyStruct>();
        ctx->SetRef("Test Window");
        IM_CHECK_EQ(vars.MyInt, 0);
        ctx->MouseMove("MyInt", ImGuiTestOpFlags_MoveToEdgeL);
        ctx->MouseDown(0);
        ctx->MouseMove("MyInt", ImGuiTestOpFlags_MoveToEdgeR);
        ctx->MouseUp(0);
        IM_CHECK_EQ(vars.MyInt, 100); // Clamped

        IM_CHECK_EQ(vars.MyFloat, 0.0f);
        ctx->MouseMove("MyFloat", ImGuiTestOpFlags_MoveToEdgeL);
        ctx->MouseDown(0);
        ctx->MouseMove("MyFloat", ImGuiTestOpFlags_MoveToEdgeR);
        IM_CHECK_EQ(vars.MyFloat, 1.0f); // Clamped
        };

    //-----------------------------------------------------------------
    // ## ImGui::Reflect - drag with min/max
    //   - Test clamping and drag behavior
    //-----------------------------------------------------------------

    t = IM_REGISTER_TEST(e, "imgui_reflect", "drag min/max");
    t->SetVarsDataType<MyStruct>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        MyStruct& vars = ctx->GetVars<MyStruct>();
        ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_Always);
        ImGui::Begin("Test Window", nullptr, ImGuiWindowFlags_NoSavedSettings);

        ImSettings settings;
        settings.push<int>()
            .min(-100)
            .max(100)
            .as_drag()
            .pop()
            .push<float>()
            .min(-1.0f)
            .max(1.0f)
            .as_drag()
            .pop();

        ImGui::Reflect::Input("MyStruct", vars, settings);
        ImGui::End();
        };

    t->TestFunc = [](ImGuiTestContext* ctx) {
        MyStruct& vars = ctx->GetVars<MyStruct>();
        ctx->SetRef("Test Window");

        IM_CHECK_EQ(vars.MyInt, 0);
        ctx->MouseMove("MyInt", ImGuiTestOpFlags_MoveToEdgeL);
        ctx->MouseDown(0);
        ctx->MouseMove("MyInt", ImGuiTestOpFlags_MoveToEdgeR);
        ctx->MouseUp(0);
        IM_CHECK_EQ(vars.MyInt, 100); // Clamped

        IM_CHECK_EQ(vars.MyFloat, 0.0f);
        ctx->MouseMove("MyFloat", ImGuiTestOpFlags_MoveToEdgeL);
        ctx->MouseDown(0);
        ctx->MouseMove("MyFloat", ImGuiTestOpFlags_MoveToEdgeR);
        IM_CHECK_EQ(vars.MyFloat, 1.0f); // Clamped
        };

    //-----------------------------------------------------------------
    // ## ImGui::Reflect - slider number limits
    //  - Test number limits and slider behavior
    //-----------------------------------------------------------------
    t = IM_REGISTER_TEST(e, "imgui_reflect", "slider number limits");
    t->SetVarsDataType<MyStruct>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        MyStruct& vars = ctx->GetVars<MyStruct>();
        ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_Always);
        ImGui::Begin("Test Window", nullptr, ImGuiWindowFlags_NoSavedSettings);
        ImSettings settings;
        settings.push<int>()
            .as_slider()
            .pop()
            .push<float>()
            .as_slider()
            .pop();
        ImGui::Reflect::Input("MyStruct", vars, settings);
        ImGui::End();
        };

    t->TestFunc = [](ImGuiTestContext* ctx) {
        MyStruct& vars = ctx->GetVars<MyStruct>();
        ctx->SetRef("Test Window");
        IM_CHECK_EQ(vars.MyInt, 0);
        ctx->MouseMove("MyInt", ImGuiTestOpFlags_MoveToEdgeL);
        ctx->MouseDown(0);
        ctx->MouseMove("MyInt", ImGuiTestOpFlags_MoveToEdgeR);
        ctx->MouseUp(0);
        IM_CHECK_EQ(vars.MyInt, INT_MAX / 2); // Clamped
        IM_CHECK_EQ(vars.MyFloat, 0.0f);
        ctx->MouseMove("MyFloat", ImGuiTestOpFlags_MoveToEdgeL);
        ctx->MouseDown(0);
        ctx->MouseMove("MyFloat", ImGuiTestOpFlags_MoveToEdgeR);
        IM_CHECK_EQ(vars.MyFloat, FLT_MAX / 2); // Clamped
        };
    //-----------------------------------------------------------------
// ## imgui_reflect - Hexadecimal Input
//  - Test hex input parsing with as_hex()
//-----------------------------------------------------------------
    t = IM_REGISTER_TEST(e, "imgui_reflect", "hexadecimal input");
    t->SetVarsDataType<int>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        int& value = ctx->GetVars<int>();
        ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_Always);
        ImGui::Begin("Test Window", nullptr, ImGuiWindowFlags_NoSavedSettings);
        ImSettings settings;
        settings.push<int>()
            .as_hex()
            .pop();
        ImGui::Reflect::Input("Value", value, settings);
        ImGui::End();
        };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        int& value = ctx->GetVars<int>();
        ctx->SetRef("Test Window");
        IM_CHECK_EQ(value, 0);
        ctx->ItemInputValue("Value", "ff");
        IM_CHECK_EQ(value, 255);
        ctx->ItemInputValue("Value", "A0");
        IM_CHECK_EQ(value, 160);
        ctx->ItemInputValue("Value", "1ff");
        IM_CHECK_EQ(value, 511);
        ctx->ItemInputValue("Value", "0");
        IM_CHECK_EQ(value, 0);
        };

    //-----------------------------------------------------------------
    // ## imgui_reflect - Hexadecimal Uppercase Input
    //  - Test hex uppercase input parsing
    //-----------------------------------------------------------------
    t = IM_REGISTER_TEST(e, "imgui_reflect", "hexadecimal uppercase input");
    t->SetVarsDataType<int>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        int& value = ctx->GetVars<int>();
        ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_Always);
        ImGui::Begin("Test Window", nullptr, ImGuiWindowFlags_NoSavedSettings);
        ImSettings settings;
        settings.push<int>()
            .as_hex(true)
            .pop();
        ImGui::Reflect::Input("Value", value, settings);
        ImGui::End();
        };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        int& value = ctx->GetVars<int>();
        ctx->SetRef("Test Window");
        IM_CHECK_EQ(value, 0);
        ctx->ItemInputValue("Value", "FF");
        IM_CHECK_EQ(value, 255);
        ctx->ItemInputValue("Value", "ff");  // Should still work with lowercase
        IM_CHECK_EQ(value, 255);
        ctx->ItemInputValue("Value", "DEADBEEF");
        IM_CHECK_EQ(value, 0xDEADBEEF);
        };

    //-----------------------------------------------------------------
    // ## imgui_reflect - Octal Input
    //  - Test octal input parsing with as_octal()
    //-----------------------------------------------------------------
    t = IM_REGISTER_TEST(e, "imgui_reflect", "octal input");
    t->SetVarsDataType<int>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        int& value = ctx->GetVars<int>();
        ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_Always);
        ImGui::Begin("Test Window", nullptr, ImGuiWindowFlags_NoSavedSettings);
        ImSettings settings;
        settings.push<int>()
            .as_octal()
            .pop();
        ImGui::Reflect::Input("Value", value, settings);
        ImGui::End();
        };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        int& value = ctx->GetVars<int>();
        ctx->SetRef("Test Window");
        IM_CHECK_EQ(value, 0);
        ctx->ItemInputValue("Value", "77");
        IM_CHECK_EQ(value, 63); // 77 octal = 63 decimal
        ctx->ItemInputValue("Value", "100");
        IM_CHECK_EQ(value, 64); // 100 octal = 64 decimal
        ctx->ItemInputValue("Value", "777");
        IM_CHECK_EQ(value, 511); // 777 octal = 511 decimal
        };

    //-----------------------------------------------------------------
    // ## imgui_reflect - Float Precision Input
    //  - Test float input with different precisions
    //-----------------------------------------------------------------
    t = IM_REGISTER_TEST(e, "imgui_reflect", "float precision input");
    t->SetVarsDataType<float>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        float& value = ctx->GetVars<float>();
        ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_Always);
        ImGui::Begin("Test Window", nullptr, ImGuiWindowFlags_NoSavedSettings);
        ImSettings settings;
        settings.push<float>()
            .as_float(2)  // 2 decimal places
            .pop();
        ImGui::Reflect::Input("Value", value, settings);
        ImGui::End();
        };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        float& value = ctx->GetVars<float>();
        ctx->SetRef("Test Window");
        IM_CHECK_EQ(value, 0.0f);
        ctx->ItemInputValue("Value", "3.14159");
        IM_CHECK_EQ(value, 3.14159f, 0.001f); // Input should be accepted fully
        ctx->ItemInputValue("Value", "2.5");
        IM_CHECK_EQ(value, 2.5f, 0.001f);
        ctx->ItemInputValue("Value", "10");
        IM_CHECK_EQ(value, 10.0f, 0.001f);
        };

    //-----------------------------------------------------------------
    // ## imgui_reflect - Scientific Notation Input
    //  - Test scientific notation input parsing
    //-----------------------------------------------------------------
    t = IM_REGISTER_TEST(e, "imgui_reflect", "scientific notation input");
    t->SetVarsDataType<double>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        double& value = ctx->GetVars<double>();
        ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_Always);
        ImGui::Begin("Test Window", nullptr, ImGuiWindowFlags_NoSavedSettings);
        ImSettings settings;
        settings.push<double>()
            .as_scientific(3)
            .pop();
        ImGui::Reflect::Input("Value", value, settings);
        ImGui::End();
        };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        double& value = ctx->GetVars<double>();
        ctx->SetRef("Test Window");
        IM_CHECK_EQ(value, 0.0);
        ctx->ItemInputValue("Value", "1.23e4");
        IM_CHECK_EQ(value, 12300.0, 0.1);
        ctx->ItemInputValue("Value", "2.5E-3");
        IM_CHECK_EQ(value, 0.0025, 0.0001);
        ctx->ItemInputValue("Value", "1e10");
        IM_CHECK_EQ(value, 1e10, 1000);
        };

    //-----------------------------------------------------------------
    // ## imgui_reflect - Decimal Input with Sign Display
    //  - Test that sign formatting doesn't affect input parsing
    //-----------------------------------------------------------------
    t = IM_REGISTER_TEST(e, "imgui_reflect", "signed decimal input");
    t->SetVarsDataType<int>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        int& value = ctx->GetVars<int>();
        ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_Always);
        ImGui::Begin("Test Window", nullptr, ImGuiWindowFlags_NoSavedSettings);
        ImSettings settings;
        settings.push<int>()
            .always_show_sign()
            .pop();
        ImGui::Reflect::Input("Value", value, settings);
        ImGui::End();
        };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        int& value = ctx->GetVars<int>();
        ctx->SetRef("Test Window");
        IM_CHECK_EQ(value, 0);
        ctx->ItemInputValue("Value", "42");
        IM_CHECK_EQ(value, 42);
        ctx->ItemInputValue("Value", "-25");
        IM_CHECK_EQ(value, -25);
        ctx->ItemInputValue("Value", "+100");
        IM_CHECK_EQ(value, 100);
        };

    //-----------------------------------------------------------------
    // ## imgui_reflect - Zero Padded Input
    //  - Test that zero padding doesn't affect input parsing
    //-----------------------------------------------------------------
    t = IM_REGISTER_TEST(e, "imgui_reflect", "zero padded input");
    t->SetVarsDataType<int>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        int& value = ctx->GetVars<int>();
        ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_Always);
        ImGui::Begin("Test Window", nullptr, ImGuiWindowFlags_NoSavedSettings);
        ImSettings settings;
        settings.push<int>()
            .zero_pad(6)
            .pop();
        ImGui::Reflect::Input("Value", value, settings);
        ImGui::End();
        };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        int& value = ctx->GetVars<int>();
        ctx->SetRef("Test Window");
        IM_CHECK_EQ(value, 0);
        ctx->ItemInputValue("Value", "42");
        IM_CHECK_EQ(value, 42);
        ctx->ItemInputValue("Value", "000123");
        IM_CHECK_EQ(value, 123); // Leading zeros should be ignored for input
        ctx->ItemInputValue("Value", "7");
        IM_CHECK_EQ(value, 7);
        };

    //-----------------------------------------------------------------
    // ## imgui_reflect - Prefix/Suffix Input
    //  - Test that prefix/suffix doesn't interfere with input parsing
    //-----------------------------------------------------------------
    t = IM_REGISTER_TEST(e, "imgui_reflect", "prefix suffix input");
    t->SetVarsDataType<int>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        int& value = ctx->GetVars<int>();
        ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_Always);
        ImGui::Begin("Test Window", nullptr, ImGuiWindowFlags_NoSavedSettings);
        ImSettings settings;
        settings.push<int>()
            .prefix("Value: ")
            .suffix(" units")
            .as_slider()
            .pop();
        ImGui::Reflect::Input("Value", value, settings);
        ImGui::End();
        };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        int& value = ctx->GetVars<int>();
        ctx->SetRef("Test Window");
        IM_CHECK_EQ(value, 0);
        ctx->ItemInputValue("Value", "123");
        IM_CHECK_EQ(value, 123);
        ctx->ItemInputValue("Value", "-45");
        IM_CHECK_EQ(value, -45);
        };

    //-----------------------------------------------------------------
    // ## imgui_reflect - Hex with Prefix Input
    //  - Test hex input with "0x" prefix
    //-----------------------------------------------------------------
    t = IM_REGISTER_TEST(e, "imgui_reflect", "hex with prefix input");
    t->SetVarsDataType<int>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        int& value = ctx->GetVars<int>();
        ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_Always);
        ImGui::Begin("Test Window", nullptr, ImGuiWindowFlags_NoSavedSettings);
        ImSettings settings;
        settings.push<int>()
            .as_hex()
            .prefix("0x")
            .as_drag()
            .pop();
        ImGui::Reflect::Input("Value", value, settings);
        ImGui::End();
        };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        int& value = ctx->GetVars<int>();
        ctx->SetRef("Test Window");
        IM_CHECK_EQ(value, 0);
        ctx->ItemInputValue("Value", "ff");      // Should work without 0x
        IM_CHECK_EQ(value, 255);
        ctx->ItemInputValue("Value", "0xff");    // Should work with 0x
        IM_CHECK_EQ(value, 255);
        ctx->ItemInputValue("Value", "A0");
        IM_CHECK_EQ(value, 160);
        };

    //-----------------------------------------------------------------
    // ## imgui_reflect - Mixed Format Complex Input
    //  - Test complex input with multiple format settings
    //-----------------------------------------------------------------
    t = IM_REGISTER_TEST(e, "imgui_reflect", "complex format input");
    struct ComplexData {
        int hex_val = 0;
        float percent_val = 0.0f;
        double sci_val = 0.0;
    };
    t->SetVarsDataType<ComplexData>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ComplexData& data = ctx->GetVars<ComplexData>();
        ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_Always);
        ImGui::Begin("Test Window", nullptr, ImGuiWindowFlags_NoSavedSettings);

        ImSettings hex_settings;
        hex_settings.push<int>()
            .as_hex()
            .prefix("0x")
            .as_drag()
            .pop();
        ImGui::Reflect::Input("HexValue", data.hex_val, hex_settings);

        ImSettings percent_settings;
        percent_settings.push<float>()
            .as_percentage(2)
            .pop();
        ImGui::Reflect::Input("PercentValue", data.percent_val, percent_settings);

        ImSettings sci_settings;
        sci_settings.push<double>()
            .as_scientific(2)
            .pop();
        ImGui::Reflect::Input("SciValue", data.sci_val, sci_settings);

        ImGui::End();
        };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        ComplexData& data = ctx->GetVars<ComplexData>();
        ctx->SetRef("Test Window");

        // Test hex input
        ctx->ItemInputValue("HexValue", "ff");
        IM_CHECK_EQ(data.hex_val, 255);

        // Test percentage input
        ctx->ItemInputValue("PercentValue", "75.50");
        IM_CHECK_EQ(data.percent_val, 75.50);

        // Test scientific input
        ctx->ItemInputValue("SciValue", "1.23e-4");
        IM_CHECK_EQ(data.sci_val, 0.000123, 0.000001);
        };

    //-----------------------------------------------------------------
    // ## imgui_reflect - Character Input
    //  - Test character input and display
    //-----------------------------------------------------------------
    t = IM_REGISTER_TEST(e, "imgui_reflect", "character input");
    t->SetVarsDataType<char>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        char& value = ctx->GetVars<char>();
        ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_Always);
        ImGui::Begin("Test Window", nullptr, ImGuiWindowFlags_NoSavedSettings);
        ImSettings settings;
        settings.push<char>()
            .as_char()
            .pop();
        ImGui::Reflect::Input("Value", value, settings);
        ImGui::End();
        };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        char& value = ctx->GetVars<char>();
        ctx->SetRef("Test Window");
        IM_CHECK_EQ(value, 0);
        ctx->ItemInputValue("Value", "A");
        IM_CHECK_EQ(value, 'A');
        ctx->ItemInputValue("Value", "z");
        IM_CHECK_EQ(value, 'z');
        ctx->ItemInputValue("Value", "5");
        IM_CHECK_EQ(value, '5');
        };
}
