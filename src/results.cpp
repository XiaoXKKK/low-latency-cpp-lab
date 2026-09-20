#include "lab/benchmark.hpp"
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
namespace lab {
namespace {
std::string quote(const std::string& text) {
    std::ostringstream out; out << '"';
    for(unsigned char ch : text) {
        if(ch=='"' || ch=='\\') out << '\\' << ch;
        else if(ch<32) out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(ch) << std::dec;
        else out << ch;
    }
    out << '"'; return out.str();
}
std::string number(double value) {
    if(!std::isfinite(value)) throw std::runtime_error("non-finite result");
    std::ostringstream out; out<<std::setprecision(12)<<value; return out.str();
}
std::string metrics_json(const std::map<std::string,double>& metrics) {
    std::string out="{"; bool first=true;
    for(const auto& [key,value]:metrics) { if(!first) out+=','; first=false; out+=quote(key)+":"+number(value); }
    return out+'}';
}
std::string csv_quote(const std::string& value) {
    std::string out="\""; for(char ch:value) { if(ch=='"') out+='"'; out+=ch; } return out+'"';
}
}
Result unavailable(const Config& c,std::string name,std::string variant,std::string reason) {
    Result result; result.benchmark=std::move(name); result.variant=std::move(variant);
    result.status="NOT MEASURED"; result.notes=std::move(reason); result.threads=c.threads; return result;
}
void output(const Config& c,const Results& results) {
    if(c.format=="json") {
        std::cout<<"{\"schema_version\":2,\"compiler\":"<<quote(__VERSION__)<<",\"build_type\":"<<quote(LAB_BUILD_TYPE)
                 <<",\"sanitizer\":"<<quote(LAB_SANITIZER)<<",\"config\":{\"iterations\":"<<c.iterations<<",\"warmup\":"<<c.warmup
                 <<",\"batch\":"<<c.batch<<",\"size_bytes\":"<<c.size<<",\"threads\":"<<c.threads<<",\"duration_seconds\":"<<number(c.duration)
                 <<",\"timeout_ms\":"<<c.timeout_ms<<",\"seed\":"<<c.seed<<",\"critical\":"<<c.critical<<",\"locks\":"<<c.locks
                 <<",\"stride\":"<<c.stride<<",\"distance\":"<<c.distance<<",\"interval_ns\":"<<c.interval_ns<<",\"read_percent\":"<<c.read_percent
                 <<",\"memory_node\":"<<c.memory_node<<",\"touch_cpu\":"<<c.touch_cpu<<",\"background_cpu\":"<<c.background_cpu<<",\"cpus\":[";
        for(std::size_t i=0;i<c.cpus.size();++i) std::cout<<(i?",":"")<<c.cpus[i];
        std::cout<<"]},\"cache_line_bytes\":"<<cache_line_size()<<",\"results\":[";
    } else {
        std::vector<std::string> columns={"benchmark","variant","mode","sample_kind","unit","threads","samples","operations_per_sample","mean","median","min","max","p50","p90","p95","p99","p999","stddev","ops_per_sec","status","metrics","notes"};
        for(std::size_t i=0;i<columns.size();++i) std::cout<<(i?(c.format=="csv"?",":" | "):"")<<columns[i];
        std::cout<<'\n';
    }
    for(std::size_t index=0;index<results.size();++index) {
        const auto& r=results[index]; const bool measured=r.status=="MEASURED";
        if(measured && r.samples.empty()) throw std::runtime_error("MEASURED result has no samples");
        Stats s=measured?statistics(r.samples):Stats{};
        const bool rate=measured && r.mode=="throughput" && r.total_ns>0;
        const auto throughput=rate?number(r.total_operations*1e9/r.total_ns):"null";
        std::vector<std::pair<std::string,double>> stats={{"mean",s.mean},{"median",s.median},{"min",s.min},{"max",s.max},{"p50",s.p50},{"p90",s.p90},{"p95",s.p95},{"p99",s.p99},{"p999",s.p999},{"stddev",s.stddev}};
        if(c.format=="json") {
            if(index) std::cout<<',';
            std::cout<<"{\"benchmark\":"<<quote(r.benchmark)<<",\"variant\":"<<quote(r.variant)<<",\"mode\":"<<quote(r.mode)
                     <<",\"sample_kind\":"<<quote(r.sample_kind)<<",\"unit\":"<<quote(r.unit)<<",\"threads\":"<<r.threads
                     <<",\"sample_count\":"<<r.samples.size()<<",\"operations_per_sample\":"<<r.operations_per_sample;
            for(auto [key,value]:stats) std::cout<<','<<quote(key)<<':'<<(measured?number(value):"null");
            std::cout<<",\"ops_per_sec\":"<<throughput<<",\"status\":"<<quote(r.status)<<",\"metrics\":"<<metrics_json(r.metrics)<<",\"notes\":"<<quote(r.notes)<<",\"raw_samples\":[";
            for(std::size_t j=0;j<r.samples.size();++j) std::cout<<(j?",":"")<<number(r.samples[j]);
            std::cout<<"]}";
        } else {
            std::vector<std::string> fields={r.benchmark,r.variant,r.mode,r.sample_kind,r.unit,std::to_string(r.threads),std::to_string(r.samples.size()),std::to_string(r.operations_per_sample)};
            for(auto [key,value]:stats) { (void)key; fields.push_back(measured?number(value):""); }
            fields.push_back(rate?throughput:""); fields.push_back(r.status); fields.push_back(metrics_json(r.metrics)); fields.push_back(r.notes);
            for(std::size_t i=0;i<fields.size();++i) std::cout<<(i?(c.format=="csv"?",":" | "):"")<<(c.format=="csv"?csv_quote(fields[i]):fields[i]);
            std::cout<<'\n';
        }
    }
    if(c.format=="json") std::cout<<"]}\n";
}
}
