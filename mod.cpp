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

#include <cctype>
#include <functional>
#include <sstream>

namespace mkn::clang {

class LLVM_OptRec_Module : public mkn::mod::Module {
  // Derives opt-viewer.py's path from the compiler's own resource dir, e.g.
  // clang --print-resource-dir -> /usr/lib/llvm-22/lib/clang/22, three levels
  // up is the install prefix, which ships share/opt-viewer/opt-viewer.py.
  static std::string guess_viewer_bin(std::string const& compiler) {
    try {
      mkn::kul::Process p{compiler};
      mkn::kul::ProcessCapture pc{p};
      p << "-print-resource-dir";
      p.start();

      std::string resourceDir = pc.outs();
      while (!resourceDir.empty() && std::isspace(static_cast<unsigned char>(resourceDir.back())))
        resourceDir.pop_back();
      if (resourceDir.empty()) return "";

      mkn::kul::Dir const prefix = mkn::kul::Dir(resourceDir).parent().parent().parent();
      mkn::kul::File const guess{"opt-viewer.py",
                                  mkn::kul::Dir{"opt-viewer", mkn::kul::Dir{"share", prefix}}};
      return guess ? guess.real() : "";
    } catch (...) {
      return "";
    }
  }

 public:
  void link(mkn::mod::Context& ctx, YAML::Node const& node) KTHROW(std::exception) override {
    std::string const buildDir = ctx.state().get("buildDir", ".");

    mkn::kul::Dir const res{"res", buildDir};
    res.mk();
    mkn::kul::Dir const tmp{"tmp", buildDir};
    tmp.mk();

    std::string compiler;
    ctx.per_compiler_command([&](mkn::mod::CompileCommand const& cmd) {
      mkn::kul::File const inFile{cmd.in};
      std::stringstream ss;
      ss << std::hex << std::hash<std::string>()(inFile.dir().real());
      std::string const base = ss.str() + "_" + inFile.name();
      mkn::kul::File const record{base + ".opt.yaml", res};
      mkn::kul::File const obj{base + ".o", tmp};

      std::string const full = ctx.compileCommandFor(cmd.in);
      auto const firstSpace = full.find(' ');
      compiler = full.substr(0, firstSpace);
      std::string const flags = full.substr(firstSpace + 1, full.rfind(" -o") - (firstSpace + 1));

      mkn::kul::Process p{compiler};
      for (std::string const& a : mkn::kul::cli::asArgs(flags)) p << a;
      p << "-fsave-optimization-record" << ("-foptimization-record-file=" + record.mini());
      p << "-o" << obj.mini() << "-c" << cmd.in;
      KLOG(DBG) << p;
      p.start();
    });

    std::string const viewer_bin = [&]() -> std::string {
      if (node["bin"]) return node["bin"].Scalar();
      if (std::string const env = mkn::kul::env::GET("OPT_VIEWER"); !env.empty()) return env;
      if (std::string const guess = guess_viewer_bin(compiler.empty() ? "clang" : compiler);
          !guess.empty())
        return guess;
      return "/usr/share/opt-viewer/opt-viewer.py";
    }();

    if (!mkn::kul::File(viewer_bin))
      KEXCEPT(mkn::kul::Exception, "opt-viewer.py not found at \"" + viewer_bin +
                                        "\", set via the \"bin\" option or $OPT_VIEWER");

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
