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
#include "mkn/mod/init.hpp"  // IWYU pragma: keep

#include "mkn/kul/log.hpp"
#include "mkn/kul/os.hpp"
#include "mkn/kul/proc.hpp"

#include <functional>
#include <sstream>

namespace mkn::clang {

class LLVM_OptRec_Module : public mkn::mod::Module {
 public:
  void link(mkn::mod::Context& ctx, YAML::Node const& node) KTHROW(std::exception) override {
    std::string const viewer_bin =
        node["bin"] ? node["bin"].Scalar()
                    : mkn::kul::env::GET("OPT_VIEWER", "/usr/share/opt-viewer/opt-viewer.py");
    std::string const buildDir = ctx.state().get("buildDir", ".");

    mkn::kul::Dir const res{"res", buildDir};
    res.mk();
    mkn::kul::Dir const tmp{"tmp", buildDir};
    tmp.mk();

    ctx.per_compiler_command([&](mkn::mod::CompileCommand const& cmd) {
      mkn::kul::File const inFile{cmd.in};
      std::stringstream ss;
      ss << std::hex << std::hash<std::string>()(inFile.dir().real());
      std::string const base = ss.str() + "_" + inFile.name();
      mkn::kul::File const record{base + ".opt.yaml", res};
      mkn::kul::File const obj{base + ".o", tmp};

      std::string const full = ctx.compileCommandFor(cmd.in);
      auto const firstSpace = full.find(' ');
      std::string const compiler = full.substr(0, firstSpace);
      std::string const flags = full.substr(firstSpace + 1, full.rfind(" -o") - (firstSpace + 1));

      mkn::kul::Process p{compiler};
      for (std::string const& a : mkn::kul::cli::asArgs(flags)) p << a;
      p << "-fsave-optimization-record" << ("-foptimization-record-file=" + record.mini());
      p << "-o" << obj.mini() << "-c" << cmd.in;
      KLOG(DBG) << p;
      p.start();
    });

    mkn::kul::Dir const html{"res_html", buildDir};
    html.mk();

    mkn::kul::Process p{viewer_bin};
    p << res.mini() << "--output-dir" << html.mini();
    KLOG(DBG) << p;
    p.start();
  }
};

}  // namespace mkn::clang

extern "C" MKN_KUL_PUBLISH mkn::mod::Module* maiken_module_construct() {
  return new mkn ::clang ::LLVM_OptRec_Module;
}

extern "C" MKN_KUL_PUBLISH void maiken_module_destruct(mkn::mod::Module* p) { delete p; }
