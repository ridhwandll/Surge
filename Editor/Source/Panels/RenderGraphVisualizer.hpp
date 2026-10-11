// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Panels/RenderGraphTarget.hpp"
#include "Surge/Core/Vector.hpp"
#include "Surge/Graphics/RenderGraph/RenderGraph.hpp"
#include <imgui.h>

namespace Surge
{
    // Draws a RenderGraph (execution groups, passes, barriers and the images between them) with a pass inspector (reads/writes + previews)
    // Hosted by the Renderer panel's "Graph" tab
    class RenderGraphVisualizer
    {
    public:
        void Draw(RenderGraph& graph, const RenderGraphTarget& target);

    private:
        struct GroupLayout
        {
            ImVec2 Min, Max;
        };

        struct GraphEdge
        {
            size_t Src, Dst;               // Indices into the compiled groups
            size_t FirstImage, ImageCount; // Range into mEdgeImages
            int Lane;                      // -1 = straight edge between neighbouring groups, otherwise a routing lane right of the groups
            float ExitY, EntryY;
        };

        void DrawToolbar(RenderGraph& graph, const RenderGraphTarget& target);
        void DrawGraph(RenderGraph& graph);
        void DrawInspector(RenderGraph& graph);
        void DrawImageList(RenderGraph& graph, const char* title, const Vector<ImageHandle>& images, bool isReads, const RenderPass* self);
        void DrawPassLinks(RenderGraph& graph, const char* label, ImageHandle image, bool producers, const RenderPass* self);

        RenderPass* FindSelectedPass(RenderGraph& graph, size_t& outGroup, int& outOrder) const;
        void SetPassEnabled(RenderPass* pass, bool enabled);
        const char* ImageName(ImageHandle h) const;

    private:
        GraphicsRHI* mRHI = nullptr;

        String mSelectedPass;     // By name: the selection follows when switching graphs (every view runs the same passes)
        float mPreviewSize = 220.0f;
        bool mLinkToggles = true; // Enabling/disabling a pass applies to every graph

        // Layout scratch, reused every frame
        Vector<GroupLayout> mLayouts;
        Vector<GraphEdge> mEdges;
        Vector<ImageBarrier> mEdgeImages;
    };

} // namespace Surge
