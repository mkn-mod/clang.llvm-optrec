/**
Copyright (c) 2024, Philip Deegan.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

    * Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the
distribution.
    * Neither the name of Philip Deegan nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <string_view>
#include <unordered_set>

#include "maiken/module/init.hpp"

namespace mkn::clang {

class AppHack : public maiken::Application {
  std::string_view constexpr static base0 =
      " -fsave-optimization-record -foptimization-record-file=";

public:
  auto update(maiken::Source const &s) {
    mkn::kul::Dir res{"res", this->buildDir()};
    mkn::kul::File inFile{s.in()};
    std::stringstream ss;
    ss << std::hex << std::hash<std::string>()(inFile.dir().real());
    mkn::kul::File yaml{ss.str() + "_" + inFile.name() + ".opt.yaml", res};
    std::string arg = s.args() + std::string{base0} + yaml.mini();
    return maiken::Source{s.in(), arg};
  }
  void hack() {
    auto const sourceMap = this->sourceMap();
    std::vector<std::pair<maiken::Source, bool>> sources;
    for (auto const &[k0, m0] : sourceMap) {
      for (auto const &[k1, v0] : m0) {
        for (auto const &sss : v0) {
          sources.emplace_back(std::make_pair(update(sss), false));
        }
      }
    }

    this->srcs = sources;
    if (this->main_)
      this->main_ = update(*this->main_);
  }
};

// todo - better opt-viewer finding - eg
// std::string viewer = "/usr/lib/llvm-14/share/opt-viewer/opt-viewer.py";

class LLVM_OptRec_Module : public maiken::Module {

public:
  void init(maiken::Application &a, YAML::Node const &node)
      KTHROW(std::exception) override {
    a.buildDir().mk();
    mkn::kul::Dir{"res", a.buildDir()}.mk();
  }

  void compile(maiken::Application &a, YAML::Node const &node)
      KTHROW(std::exception) override {
    reinterpret_cast<AppHack *>(&a)->hack();
  }

  void link(maiken::Application &a, YAML::Node const &node)
      KTHROW(std::exception) override {
    mkn::kul::Dir res{"res", a.buildDir()};
    mkn::kul::Dir hmtl{"res_html", a.buildDir()};
    hmtl.mk();

    mkn::kul::Process p{"/usr/lib/llvm-14/share/opt-viewer/opt-viewer.py"};
    p << res.mini() << "--output-dir" << hmtl.mini();
    KLOG(DBG) << p;
    p.start();
  }
};

} // namespace mkn::clang

extern "C" KUL_PUBLISH maiken::Module *maiken_module_construct() {
  return new mkn ::clang ::LLVM_OptRec_Module;
}

extern "C" KUL_PUBLISH void maiken_module_destruct(maiken::Module *p) {
  delete p;
}
