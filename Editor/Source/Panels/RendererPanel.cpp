// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Panels/RendererPanel.hpp"
#include "Surge/Core/Core.hpp"
#include "Surge/Graphics/RHI/RHI.hpp"
#include "Surge/Graphics/Renderer/Renderer.hpp"
#include "Surge/Graphics/RenderGraph/Passes/GeometryPass.hpp"
#include "Utility/ImGuiAux.hpp"

#include <imgui.h>
#include <algorithm>
#include <array>

namespace Surge
{
    namespace
    {
        constexpr std::array<const char*, 4> QUALITY_NAMES = { "Low (Mobile)", "Medium", "High", "Ultra (Desktop)" };

        bool BeginSection(const char* name, bool openByDefault = true)
        {
            if(!ImGuiAux::PropertyGridHeader(name, openByDefault))
                return false;

            if(!ImGui::BeginTable("##Properties", 2, ImGuiTableFlags_SizingStretchProp))
            {
                ImGui::TreePop();
                return false;
            }
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 120.0f);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
            return true;
        }

        void EndSection()
        {
            ImGui::EndTable();
            ImGui::Dummy(ImVec2(0.0f, 2.0f));
            ImGui::TreePop();
        }

        // Label cell, then the value cell sized for the widget that follows
        void Row(const char* label)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(label);
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);
        }

        template <typename Q, typename Settings>
        void QualityRow(Settings& settings)
        {
            Row("Quality");
            int quality = static_cast<int>(settings.Quality);
            if(ImGui::Combo("##Quality", &quality, QUALITY_NAMES.data(), static_cast<int>(QUALITY_NAMES.size())))
                settings.ApplyQualityPreset(static_cast<Q>(quality));
            ImGuiAux::ItemTooltip("Sets the cost knobs (see Advanced), the look settings are kept");
        }

        // Mirrors the per-platform defaults of FrameBlackboard.hpp, so the mobile budget can be previewed in the Editor
        void ApplyPlatformBudget(FrameBlackboard& settings, bool mobile)
        {
            Core::GetRenderer()->SetRenderScale(mobile ? 0.8f : 1.0f);
            settings.Lighting.MaxLights = mobile ? 8 : 64;
            settings.ShadowSettings_.CascadeCount = mobile ? 2 : MAX_SHADOW_CASCADE_COUNT;
            settings.AOSettings.ApplyQualityPreset(mobile ? AOQuality::LOW : AOQuality::ULTRA);
            settings.AOSettings.Enable = !mobile;
            settings.Bloom.ApplyQualityPreset(mobile ? BloomQuality::LOW : BloomQuality::ULTRA);
            settings.Particles.MaxParticles = mobile ? 8192 : 131072;
            settings.Particles.EmissionQuality = 1.0f;
        }
    } // namespace

    void RendererPanel::Init(void*)
    {
        mCode = GetStaticCode();
    }

    void RendererPanel::Render(bool* show)
    {
        if(!*show)
            return;

        ImGui::SetNextWindowSize(ImVec2(420.0f, 640.0f), ImGuiCond_FirstUseEver);
        if(ImGui::Begin(PanelCodeToString(mCode), show))
        {
            if(ImGui::BeginTabBar("##RendererTabs"))
            {
                if(ImGui::BeginTabItem("Settings"))
                {
                    DrawSettings(Core::GetRenderer()->GetRenderGraphBlackBoard()); // Main blackboard: the source every view copies its settings from
                    ImGui::EndTabItem();
                }

                if(ImGui::BeginTabItem("Graph"))
                {
                    mTarget.DrawSelector(std::min(ImGui::GetContentRegionAvail().x, 240.0f)); // Before Resolve(): a new pick applies this frame
                    mVisualizer.Draw(*mTarget.Resolve(), mTarget);
                    ImGui::EndTabItem();
                }

                if(ImGui::BeginTabItem("Stats"))
                {
                    mTarget.DrawSelector(std::min(ImGui::GetContentRegionAvail().x, 240.0f));
                    DrawStats(*mTarget.Resolve());
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
        }
        ImGui::End();
    }

    void RendererPanel::DrawSettings(FrameBlackboard& settings)
    {
        ImGui::TextDisabled("Budget");
        ImGui::SameLine();
        if(ImGui::SmallButton("Mobile"))
            ApplyPlatformBudget(settings, true);
        ImGuiAux::ItemTooltip("Android defaults: 0.8x scale, 8 lights, 2 cascades, no AO, low bloom, 8k particles");
        ImGui::SameLine();
        if(ImGui::SmallButton("Desktop"))
            ApplyPlatformBudget(settings, false);
        ImGuiAux::ItemTooltip("Desktop defaults: full scale, 64 lights, 3 cascades, ultra AO and bloom, 128k particles");

        const char* advancedLabel = "Advanced";
        const float advancedWidth = ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.x + ImGui::CalcTextSize(advancedLabel).x;
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(ImGui::GetContentRegionAvail().x - advancedWidth, 0.0f));
        ImGui::Checkbox(advancedLabel, &mShowAdvanced);
        ImGuiAux::ItemTooltip("Show the knobs the quality presets drive");

        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        ImGui::TextWrapped("Applies to every viewport. Not saved, the Player uses its platform defaults.");
        ImGui::PopStyleColor();
        ImGui::Spacing();

        if(BeginSection("General"))
        {
            // Applied on release: each change rebuilds the scene targets of every graph
            if(!mEditingRenderScale)
                mRenderScale = settings.RenderScale;
            Row("Render Scale");
            ImGui::SliderFloat("##RenderScale", &mRenderScale, RenderScaleSettings::MIN_SCALE, RenderScaleSettings::MAX_SCALE, "%.2fx", ImGuiSliderFlags_AlwaysClamp);
            mEditingRenderScale = ImGui::IsItemActive();
            if(ImGui::IsItemDeactivatedAfterEdit())
                Core::GetRenderer()->SetRenderScale(mRenderScale);
            ImGuiAux::ItemTooltip("Resolution of the 3D scene (main pass, AO, bloom), UI and post-processing stay at full resolution");

            Row("Max Lights");
            int maxLights = static_cast<int>(settings.Lighting.MaxLights);
            if(ImGui::SliderInt("##MaxLights", &maxLights, 1, static_cast<int>(GeometryPass::MAX_LIGHTS), "%d", ImGuiSliderFlags_AlwaysClamp))
                settings.Lighting.MaxLights = static_cast<Uint>(maxLights);
            ImGuiAux::ItemTooltip("Most relevant lights after culling, every lit pixel loops over them");

            Row("FXAA");
            ImGui::Checkbox("##FXAA", &settings.PostProcessSettings_.EnableFXAA);
            ImGuiAux::ItemTooltip("Full screen anti-aliasing, not recommended on mobile");
            EndSection();
        }

        if(BeginSection("Particles"))
        {
            Row("Max Particles");
            int maxParticles = static_cast<int>(settings.Particles.MaxParticles);
            if(ImGui::DragInt("##MaxParticles", &maxParticles, 64.0f, 0, 1 << 20, "%d", ImGuiSliderFlags_AlwaysClamp))
                settings.Particles.MaxParticles = static_cast<Uint>(maxParticles);
            ImGuiAux::ItemTooltip("Live particles of every particle system together, emission stops at the budget");

            Row("Emission Quality");
            ImGui::SliderFloat("##EmissionQuality", &settings.Particles.EmissionQuality, ParticleSettings::MIN_EMISSION_QUALITY, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
            ImGuiAux::ItemTooltip("Scales every emission rate and burst, a quality knob for weaker devices");
            EndSection();
        }

        if(BeginSection("Shadows"))
        {
            ShadowSettings& shadows = settings.ShadowSettings_;
            // 2..MAX_SHADOW_CASCADE_COUNT
            constexpr const char* cascadeNames[] = { "2", "3" };
            static_assert(IM_ARRAYSIZE(cascadeNames) == MAX_SHADOW_CASCADE_COUNT - 1);
            Row("Cascades");
            int cascadeIndex = std::clamp(shadows.CascadeCount, 2, MAX_SHADOW_CASCADE_COUNT) - 2;
            if(ImGui::Combo("##Cascades", &cascadeIndex, cascadeNames, IM_ARRAYSIZE(cascadeNames)))
                shadows.CascadeCount = cascadeIndex + 2;
            ImGuiAux::ItemTooltip("Each cascade is a full shadow render pass");

            Row("Split Lambda");
            ImGui::SliderFloat("##SplitLambda", &shadows.CascadeSplitLambda, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
            ImGuiAux::ItemTooltip("0 = uniform splits, 1 = logarithmic (more resolution near the camera)");

            Row("Bias Constant");
            ImGui::DragFloat("##BiasConstant", &shadows.DepthBiasConstant, 0.05f, 0.0f, 100.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
            Row("Bias Slope");
            ImGui::DragFloat("##BiasSlope", &shadows.DepthBiasSlope, 0.05f, 0.0f, 100.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
            if(mShowAdvanced)
            {
                Row("Bias Clamp");
                ImGui::DragFloat("##BiasClamp", &shadows.DepthBiasClamp, 0.05f, 0.0f, 100.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
            }
            EndSection();
        }

        if(BeginSection("Ambient Occlusion"))
        {
            AmbientOcclusionSettings& ao = settings.AOSettings;
            Row("Enabled");
            ImGui::Checkbox("##Enabled", &ao.Enable);
            ImGuiAux::ItemTooltip("4 extra render passes and a stored depth buffer");

            ImGui::BeginDisabled(!ao.Enable);
            QualityRow<AOQuality>(ao);
            Row("Radius");
            ImGui::DragFloat("##Radius", &ao.Radius, 0.01f, 0.01f, 10.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
            Row("Intensity");
            ImGui::SliderFloat("##Intensity", &ao.Intensity, 0.1f, 4.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
            Row("Falloff");
            ImGui::SliderFloat("##Falloff", &ao.FalloffRange, 0.05f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
            Row("Max Distance");
            ImGui::DragFloat("##MaxDistance", &ao.MaxDistance, 0.5f, 1.0f, 1000.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
            if(mShowAdvanced)
            {
                Row("Resolution");
                ImGui::SliderFloat("##Resolution", &ao.ResolutionScale, 0.25f, 1.0f, "%.2fx", ImGuiSliderFlags_AlwaysClamp);
                Row("Slices");
                ImGui::SliderInt("##Slices", &ao.SliceCount, 1, 8, "%d", ImGuiSliderFlags_AlwaysClamp);
                Row("Steps / Slice");
                ImGui::SliderInt("##Steps", &ao.StepsPerSlice, 1, 16, "%d", ImGuiSliderFlags_AlwaysClamp);
                Row("Blur Radius");
                ImGui::SliderInt("##BlurRadius", &ao.BlurRadius, 0, 6, "%d", ImGuiSliderFlags_AlwaysClamp);
                Row("Blur Sharpness");
                ImGui::SliderFloat("##BlurSharpness", &ao.BlurSharpness, 0.0f, 32.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
                Row("Accurate Normals");
                ImGui::Checkbox("##AccurateNormals", &ao.AccurateNormals);
            }
            ImGui::EndDisabled();
            EndSection();
        }

        if(BeginSection("Bloom"))
        {
            BloomSettings& bloom = settings.Bloom;
            Row("Enabled");
            ImGui::Checkbox("##Enabled", &bloom.Enable);

            ImGui::BeginDisabled(!bloom.Enable);
            QualityRow<BloomQuality>(bloom);
            Row("Threshold");
            ImGui::DragFloat("##Threshold", &bloom.Threshold, 0.01f, 0.0f, 20.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
            ImGuiAux::ItemTooltip("Linear HDR brightness where bloom starts");
            Row("Knee");
            ImGui::SliderFloat("##Knee", &bloom.Knee, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
            Row("Intensity");
            ImGui::DragFloat("##Intensity", &bloom.Intensity, 0.01f, 0.0f, 10.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
            Row("Scatter");
            ImGui::SliderFloat("##Scatter", &bloom.Scatter, 0.05f, 0.95f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
            Row("Tint");
            ImGui::ColorEdit3("##Tint", &bloom.Tint.x);
            if(mShowAdvanced)
            {
                const bool supportsCompute = Core::GetRenderer()->GetRHI()->SupportsCompute();
                Row("Use Compute");
                ImGui::BeginDisabled(!supportsCompute);
                ImGui::Checkbox("##UseCompute", &bloom.UseCompute);
                ImGui::EndDisabled();
                ImGuiAux::ItemTooltip(supportsCompute ? "Storage images, desktop path" : "This device's queue has no compute support");
                Row("Mip Count");
                ImGui::SliderInt("##MipCount", &bloom.MipCount, BloomSettings::MIN_MIP_COUNT, BloomSettings::MAX_MIP_COUNT, "%d", ImGuiSliderFlags_AlwaysClamp);
                Row("HQ Filter");
                ImGui::Checkbox("##HQFilter", &bloom.HighQualityFilter);
                ImGuiAux::ItemTooltip("13 tap downsample + 9 tap tent upsample, otherwise dual filter");
                Row("Anti Flicker");
                ImGui::Checkbox("##AntiFlicker", &bloom.AntiFlicker);
                Row("HQ Composite");
                ImGui::Checkbox("##HQComposite", &bloom.HighQualityComposite);
                Row("Clamp");
                ImGui::DragFloat("##Clamp", &bloom.ClampValue, 10.0f, 1.0f, 65000.0f, "%.0f", ImGuiSliderFlags_AlwaysClamp);
                ImGuiAux::ItemTooltip("Max HDR value fed into bloom");
            }
            ImGui::EndDisabled();
            EndSection();
        }

        if(BeginSection("Post Process"))
        {
            PostProcessSettings& post = settings.PostProcessSettings_;
            Row("Vignette");
            ImGui::SliderFloat("##Vignette", &post.VignetteIntensity, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
            Row("Vignette Softness");
            ImGui::SliderFloat("##VignetteSoftness", &post.VignetteSoftness, 0.01f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
            Row("Grain");
            ImGui::SliderFloat("##Grain", &post.Grain, 0.0f, 0.15f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
            EndSection();
        }

        if(BeginSection("Debug", false))
        {
            Row("Show Cascades");
            ImGui::Checkbox("##ShowCascades", &settings.ShadowSettings_.ShowCascades);
            Row("Show AO Only");
            ImGui::BeginDisabled(!settings.AOSettings.Enable);
            ImGui::Checkbox("##ShowAOOnly", &settings.PostProcessSettings_.ShowAOOnly);
            ImGui::EndDisabled();
            EndSection();
        }
    }

    void RendererPanel::DrawStats(RenderGraph& graph)
    {
        if(mTarget.IsMainGraph() && !graph.IsSceneEnabled())
            ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.3f, 1.0f), "The main graph only presents ImGui in the Editor, pick a viewport.");

        if(ImGui::BeginChild("##Stats"))
        {
            // Stats of the selected graph's own frame, so read its blackboard (submission counts differ per view)
            for(const Scope<RenderPass>& pass : graph.GetPasses())
            {
                if(!pass->IsEnabled())
                    continue;
                ImGui::PushID(pass.get());
                pass->OnImGuiRender(graph.GetBlackboard());
                ImGui::PopID();
            }

            ImGui::Spacing();
            GraphicsRHI* rhi = graph.GetRHI();
            if(rhi && ImGui::CollapsingHeader("RHI"))
                rhi->ShowMetricsWindow();
        }
        ImGui::EndChild();
    }

} // namespace Surge
