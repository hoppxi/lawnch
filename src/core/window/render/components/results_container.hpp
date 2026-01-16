#pragma once

#include "component_base.hpp"

namespace Lawnch::Core::Window::Render::Components {

class ResultsContainer : public ComponentBase {
public:
  ComponentResult draw(ComponentContext &context) override;
  std::string name() const override { return "results"; }
  double get_item_height(const Config::Config &cfg) const;
  void invalidate_metrics() { metrics_valid = false; }

private:
  mutable double cached_item_height = 0;
  mutable bool metrics_valid = false;

  void update_metrics(const Config::Config &cfg) const;
  void draw_result_item(BLContext &ctx, const Config::Config &cfg,
                        const Search::SearchResult &result, double item_x,
                        double item_y, double item_w, double item_h,
                        bool is_selected, const std::string &search_text) const;
};

} // namespace Lawnch::Core::Window::Render::Components
