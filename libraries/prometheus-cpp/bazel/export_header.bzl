def _generate_dummy_export_header_impl(ctx):
    ctx.actions.expand_template(
        template = ctx.file._template,
        output = ctx.outputs.header_file,
        substitutions = {
            "{BASE_NAME}": ctx.attr.basename,
        },
    )

generate_dummy_export_header = rule(
    attrs = {
        "basename": attr.string(mandatory = True),
        "header": attr.string(mandatory = True),
        "_template": attr.label(
            allow_single_file = True,
            default = Label("@com_github_jupp0r_prometheus_cpp//bazel:dummy_export.h.tpl"),
        ),
    },
    implementation = _generate_dummy_export_header_impl,
    outputs = {"header_file": "%{header}"},
)
