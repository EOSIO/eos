**Mermaid Flowchart Code for Launcher Service README**

~~~mermaid
graph TD
T["test script (Python)"] -->|Uses| S("core.service (Python)")
    S -->|Talks with| L["Launcher Service (C++)"]
    L -->|Controls| C0[Cluster 0]
    		C0 --> N00[Node 0]
    		C0 --> N01[...]
    		C0 --> N02[Node m]
		L -->|Controls| C1[...]
		L -->|Controls| C2[Cluster N]
				C2 --> N20[Node 0]
				C2 --> N21[...]
				C2 --> N22[Node n]
~~~

~~~mermaid
graph TD
		subgraph 1. Initialize a cluster
    A("Logger") -->|registered to| B("Service")
    B -->|registered to| C("Cluster")
		end
		C --> D["2. Test on the cluster"]
~~~
