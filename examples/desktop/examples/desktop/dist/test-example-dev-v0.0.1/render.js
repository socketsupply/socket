(()=>{var Qe=Object.defineProperty;var a=(r,t)=>Qe(r,"name",{value:t,configurable:!0}),Dt=(r=>typeof require!="undefined"?require:typeof Proxy!="undefined"?new Proxy(r,{get:(t,e)=>(typeof require!="undefined"?require:t)[e]}):r)(function(r){if(typeof require!="undefined")return require.apply(this,arguments);throw new Error('Dynamic require of "'+r+'" is not supported')});var v=(r,t)=>()=>(t||r((t={exports:{}}).exports,t),t.exports);var Vt=v((ds,ti)=>{ti.exports={name:"@socketsupply/tonic",version:"14.1.0",description:"A component framework.",scripts:{"ci:test:tape-run":"esbuild --bundle test/index.js | tape-run",lint:"standard -v",test:"npm run build && standard && esbuild --bundle test/index.js | tape-run","test:open":"npm run build && esbuild --bundle test/index.js | tape-run --browser chrome --keep-open",prepare:"./scripts/build.js",build:"./scripts/build.js && npm run minify:esm && npm run minify:cjs","minify:esm":"esbuild index.esm.js --keep-names --minify --outfile=dist/tonic.min.esm.js","minify:cjs":"esbuild index.js --keep-names --minify --outfile=dist/tonic.min.js"},main:"index.js",author:"socketsupply",license:"MIT",devDependencies:{benchmark:"^2.1.4",esbuild:"^0.8.36",standard:"14.3.1","tape-run":"^8.0.0",tapzero:"0.2.2",uuid:"3.3.3"},standard:{ignore:["dist/*","test/fixtures/*"]},directories:{test:"test"},repository:{type:"git",url:"git+https://github.com/socketsupply/tonic.git"},bugs:{url:"https://github.com/socketsupply/tonic/issues"},homepage:"https://tonic.technology",dependencies:{}}});var y=v((ps,it)=>{"use strict";var F=class{constructor(t,e,i){this.isTonicTemplate=!0,this.unsafe=i,this.rawText=t,this.templateStrings=e}valueOf(){return this.rawText}toString(){return this.rawText}};a(F,"TonicTemplate");var p=class extends window.HTMLElement{constructor(){super();let t=p._states[super.id];delete p._states[super.id],this._state=t||{},this.preventRenderOnReconnect=!1,this.props={},this.elements=[...this.children],this.elements.__children__=!0,this.nodes=[...this.childNodes],this.nodes.__children__=!0,this._events()}static _createId(){return`tonic${p._index++}`}static _splitName(t){return t.match(/[A-Z][a-z0-9]*/g).join("-")}static _normalizeAttrs(t,e={}){return[...t].forEach(i=>e[i.name]=i.value),e}_checkId(){let t=super.id;if(!t){let e=this.outerHTML.replace(this.innerHTML,"...");throw new Error(`Component: ${e} has no id`)}return t}get state(){return this._checkId(),this._state}set state(t){this._state=(this._checkId(),t)}_events(){let t=Object.getOwnPropertyNames(window.HTMLElement.prototype);for(let e of this._props)t.indexOf("on"+e)!==-1&&this.addEventListener(e,this)}_prop(t){let e=this._id,i=`__${e}__${p._createId()}__`;return p._data[e]=p._data[e]||{},p._data[e][i]=t,i}_placehold(t){let e=this._id,i=`placehold:${e}:${p._createId()}__`;return p._children[e]=p._children[e]||{},p._children[e][i]=t,i}static match(t,e){return t.matches||(t=t.parentElement),t.matches(e)?t:t.closest(e)}static getPropertyNames(t){let e=[];for(;t&&t!==p.prototype;)e.push(...Object.getOwnPropertyNames(t)),t=Object.getPrototypeOf(t);return e}static add(t,e){if(!(e||t.name&&t.name.length>1))throw Error("Mangling. https://bit.ly/2TkJ6zP");if(e||(e=p._splitName(t.name).toLowerCase()),!p.ssr&&window.customElements.get(e))throw new Error(`Cannot Tonic.add(${t.name}, '${e}') twice`);if(!t.prototype||!t.prototype.isTonicComponent){let s={[t.name]:class extends p{}}[t.name];s.prototype.render=t,t=s}return t.prototype._props=p.getPropertyNames(t.prototype),p._reg[e]=t,p._tags=Object.keys(p._reg).join(),window.customElements.define(e,t),typeof t.stylesheet=="function"&&p.registerStyles(t.stylesheet),t}static registerStyles(t){if(p._stylesheetRegistry.includes(t))return;p._stylesheetRegistry.push(t);let e=document.createElement("style");p.nonce&&e.setAttribute("nonce",p.nonce),e.appendChild(document.createTextNode(t())),document.head&&document.head.appendChild(e)}static escape(t){return t.replace(p.ESC,e=>p.MAP[e])}static unsafeRawString(t,e){return new F(t,e,!0)}dispatch(t,e=null){let i={bubbles:!0,detail:e};this.dispatchEvent(new window.CustomEvent(t,i))}html(t,...e){let i=a(n=>{if(n&&n.__children__)return this._placehold(n);if(n&&n.isTonicTemplate)return n.rawText;switch(Object.prototype.toString.call(n)){case"[object HTMLCollection]":case"[object NodeList]":return this._placehold([...n]);case"[object Array]":return n.every(c=>c.isTonicTemplate&&!c.unsafe)?new F(n.join(`
`),null,!1):this._prop(n);case"[object Object]":case"[object Function]":return this._prop(n);case"[object NamedNodeMap]":return this._prop(p._normalizeAttrs(n));case"[object Number]":return`${n}__float`;case"[object String]":return p.escape(n);case"[object Boolean]":return`${n}__boolean`;case"[object Null]":return`${n}__null`;case"[object HTMLElement]":return this._placehold([n])}return typeof n=="object"&&n&&n.nodeType===1&&typeof n.cloneNode=="function"?this._placehold([n]):n},"refs"),s=[];for(let n=0;n<t.length-1;n++)s.push(t[n],i(e[n]));s.push(t[t.length-1]);let o=s.join("").replace(p.SPREAD,(n,c)=>{let l=p._data[c.split("__")[1]][c];return Object.entries(l).map(([h,d])=>{let m=h.replace(/([a-z])([A-Z])/g,"$1-$2").toLowerCase();return d===!0?m:d?`${m}="${p.escape(String(d))}"`:""}).filter(Boolean).join(" ")});return new F(o,t,!1)}scheduleReRender(t){return this.pendingReRender?this.pendingReRender:(this.pendingReRender=new Promise(e=>setTimeout(()=>{if(!this.isInDocument(this.shadowRoot||this))return;let i=this._set(this.shadowRoot||this,this.render);if(this.pendingReRender=null,i&&i.then)return i.then(()=>{this.updated&&this.updated(t),e()});this.updated&&this.updated(t),e()},0)),this.pendingReRender)}reRender(t=this.props){let e={...this.props};return this.props=typeof t=="function"?t(e):t,this.scheduleReRender(e)}handleEvent(t){this[t.type](t)}_drainIterator(t,e){return e.next().then(i=>{if(this._set(t,null,i.value),!i.done)return this._drainIterator(t,e)})}_set(t,e,i=""){for(let s of t.querySelectorAll(p._tags)){if(!s.isTonicComponent)continue;let o=s.getAttribute("id");!o||!p._refIds.includes(o)||(p._states[o]=s.state)}if(e instanceof p.AsyncFunction)return e.call(this,this.html,this.props).then(s=>this._apply(t,s));if(e instanceof p.AsyncFunctionGenerator)return this._drainIterator(t,e.call(this));e===null?this._apply(t,i):e instanceof Function&&this._apply(t,e.call(this,this.html,this.props)||"")}_apply(t,e){if(e&&e.isTonicTemplate?e=e.rawText:typeof e=="string"&&(e=p.escape(e)),typeof e=="string"){if(this.stylesheet&&(e=`<style nonce=${p.nonce||""}>${this.stylesheet()}</style>${e}`),t.innerHTML=e,this.styles){let o=this.styles();for(let n of t.querySelectorAll("[styles]"))for(let c of n.getAttribute("styles").split(/\s+/))Object.assign(n.style,o[c.trim()])}let i=p._children[this._id]||{},s=a((o,n)=>{if(o.nodeType===3){let l=o.textContent.trim();i[l]&&n(o,i[l],l)}let c=o.childNodes;if(!!c)for(let l=0;l<c.length;l++)s(c[l],n)},"walk");s(t,(o,n,c)=>{for(let l of n)o.parentNode.insertBefore(l,o);delete p._children[this._id][c],o.parentNode.removeChild(o)})}else t.innerHTML="",t.appendChild(e.cloneNode(!0))}connectedCallback(){this.root=this.shadowRoot||this,super.id&&!p._refIds.includes(super.id)&&p._refIds.push(super.id);let t=a(e=>e.replace(/-(.)/g,(i,s)=>s.toUpperCase()),"cc");for(let{name:e,value:i}of this.attributes){let s=t(e),o=this.props[s]=i;if(/__\w+__\w+__/.test(o)){let{1:n}=o.split("__");this.props[s]=p._data[n][o]}else if(/\d+__float/.test(o))this.props[s]=parseFloat(o,10);else if(o==="null__null")this.props[s]=null;else if(/\w+__boolean/.test(o))this.props[s]=o.includes("true");else if(/placehold:\w+:\w+__/.test(o)){let{1:n}=o.split(":");this.props[s]=p._children[n][o][0]}}if(this.props=Object.assign(this.defaults?this.defaults():{},this.props),this._id=this._id||p._createId(),this.willConnect&&this.willConnect(),!!this.isInDocument(this.root)){if(!this.preventRenderOnReconnect){this._source?this.innerHTML=this._source:this._source=this.innerHTML;let e=this._set(this.root,this.render);if(e&&e.then)return e.then(()=>this.connected&&this.connected())}this.connected&&this.connected()}}isInDocument(t){let e=t.getRootNode();return e===document||e.toString()==="[object ShadowRoot]"}disconnectedCallback(){this.disconnected&&this.disconnected(),delete p._data[this._id],delete p._children[this._id]}};a(p,"Tonic");p.prototype.isTonicComponent=!0;Object.assign(p,{_tags:"",_refIds:[],_data:{},_states:{},_children:{},_reg:{},_stylesheetRegistry:[],_index:0,version:typeof Dt!="undefined"?Vt().version:null,SPREAD:/\.\.\.\s?(__\w+__\w+__)/g,ESC:/["&'<>`/]/g,AsyncFunctionGenerator:async function*(){}.constructor,AsyncFunction:async function(){}.constructor,MAP:{'"':"&quot;","&":"&amp;","'":"&#x27;","<":"&lt;",">":"&gt;","`":"&#x60;","/":"&#x2F;"}});typeof it=="object"&&(it.exports=p)});var Ot=v((us,jt)=>{var B=y(),st=class extends B{defaults(){return{multiple:!1}}static stylesheet(){return`
      tonic-accordion {
        display: block;
        border: 1px solid var(--tonic-border, black);
        border-radius: 2px;
      }
    `}qs(t){return this.querySelector(t)}qsa(t){return[...this.querySelectorAll(t)]}setVisibility(t){let e=document.getElementById(t);if(!e)return;let i=this.hasAttribute("data-allow-multiple"),s=e.getAttribute("aria-expanded")==="true";if(!s&&!i){let c=this.qsa(".tonic--accordion-header button"),l=this.qsa(".tonic--accordion-panel");c.forEach(h=>h.setAttribute("aria-expanded","false")),l.forEach(h=>h.setAttribute("hidden",""))}let o=e.getAttribute("aria-controls")||`tonic--accordion-panel-${e.id}`;if(s){e.setAttribute("aria-expanded","false");let c=document.getElementById(o);c&&c.setAttribute("hidden",""),this.state.selected=null;return}e.setAttribute("aria-expanded","true");let n=document.getElementById(o);this.state.selected=t,n&&n.removeAttribute("hidden")}click(t){let e=B.match(t.target,"button");!e||this.setVisibility(e.id)}keydown(t){if(!B.match(t.target,"button.tonic--title"))return;let i=t.ctrlKey,s=t.code==="PageUp",o=t.code==="PageDown",n=t.code==="ArrowUp",c=t.code==="ArrowDown",l=t.metaKey&&t.code==="ArrowDown",h=t.metaKey&&t.code==="ArrowUp",d=i&&(s||o),m=this.qsa("button.tonic--title");if(n||c||d){let u=m.indexOf(t.target),b=o||c?1:-1,f=m.length,x=(u+f+b)%f;m[x].focus(),t.preventDefault()}if(h||l){switch(t.key){case h:m[0].focus();break;case l:m[m.length-1].focus();break}t.preventDefault()}}connected(){let t=this.state.selected||this.props.selected;!t||setTimeout(()=>this.setVisibility(t),0)}render(){let{multiple:t}=this.props;return t&&this.setAttribute("data-allow-multiple",""),this.html`
      ${this.nodes}
    `}};a(st,"TonicAccordion");var ot=class extends B{static stylesheet(){return`
      tonic-accordion-section {
        display: block;
      }

      tonic-accordion-section:not(:last-of-type) {
        border-bottom: 1px solid var(--tonic-border, black);
      }

      tonic-accordion-section .tonic--accordion-header {
        display: flex;
        margin: 0;
      }

      tonic-accordion-section .tonic--accordion-panel[hidden] {
        display: none;
      }

      tonic-accordion-section .tonic--accordion-panel {
        padding: 10px 50px 20px 20px;
      }

      tonic-accordion-section .tonic--accordion-header button {
        font-size: 14px;
        text-align: left;
        padding: 20px;
        position: relative;
        background: transparent;
        color: var(--tonic-primary);
        font-weight: bold;
        border: 0;
        -webkit-appearance: none;
        -moz-appearance: none;
        appearance: none;
        outline: none;
        width: 100%;
      }

      tonic-accordion-section .tonic--accordion-header button:focus {
        outline: none;
      }

      tonic-accordion-section .tonic--accordion-header button:focus .tonic--label {
        border-bottom: 2px solid var(--tonic-accent);
      }

      tonic-accordion-section .tonic--accordion-header .tonic--arrow {
        display: block;
        position: absolute;
        top: 0;
        right: 0;
        bottom: 0;
        width: 50px;
      }

      tonic-accordion-section .tonic--accordion-header .tonic--arrow:before {
        content: "";
        width: 8px;
        height: 8px;
        position: absolute;
        top: 50%;
        left: 50%;
        -webkit-transform: translateY(-50%) translateX(-50%) rotate(135deg);
        -moz-transform: translateY(-50%) translateX(-50%) rotate(135deg);
        transform: translateY(-50%) translateX(-50%) rotate(135deg);
        border-top: 1px solid var(--tonic-primary, #333);
        border-right: 1px solid var(--tonic-primary, #333);
      }

      tonic-accordion-section .tonic--accordion-header button[aria-expanded="true"] .tonic--arrow:before {
        -webkit-transform: translateY(-50%) translateX(-50%) rotate(315deg);
        -moz-transform: translateY(-50%) translateX(-50%) rotate(315deg);
        transform: translateY(-50%) translateX(-50%) rotate(315deg);
        margin-top: 3px;
      }
    `}render(){let{id:t,name:e,label:i}=this.props;return this.html`
      <div class="tonic--accordion-header" role="heading">
        <button
          class="tonic--title"
          id="tonic--accordion-header-${t}"
          name="${e}"
          aria-expanded="false"
          aria-controls="tonic--accordion-panel-${t}">
          <span class="tonic--label">${i}</span>
          <span class="tonic--arrow"></span>
        </button>
      </div>
      <div
        class="tonic--accordion-panel"
        id="tonic--accordion-panel-${t}"
        aria-labelledby="tonic--accordion-header-${t}"
        role="region"
        hidden>
        ${this.nodes}
      </div>
    `}};a(ot,"TonicAccordionSection");jt.exports={TonicAccordion:st,TonicAccordionSection:ot}});var Yt=v((hs,Ft)=>{var ei=y(),nt=class extends ei{defaults(){return{count:0}}get value(){return this.state.count}set value(t){this.state.count=t,this.reRender()}static stylesheet(){return`
      tonic-badge * {
        box-sizing: border-box;
      }

      tonic-badge .tonic--notifications {
        width: 40px;
        height: 40px;
        text-align: center;
        padding: 10px;
        position: relative;
        background-color: var(--tonic-background, #fff);
        border: 1px solid var(--tonic-input-border, transparent);
        border-radius: 8px;
      }

      tonic-badge span:after {
        content: '';
        width: 8px;
        height: 8px;
        display: none;
        position: absolute;
        top: 7px;
        right: 6px;
        background-color: var(--tonic-notification, #f66);
        border: 2px solid var(--tonic-background, #fff);
        border-radius: 50%;
      }

      tonic-badge .tonic--notifications.tonic--new span:after {
        display: block;
      }

      tonic-badge span {
        color: var(--tonic-primary, #333);
        font: 15px var(--tonic-subheader, 'Arial', sans-serif);
        letter-spacing: 1px;
        text-align: center;
      }
    `}willConnect(){this.state.count=this.props.count}render(){let{theme:t}=this.props,e=this.state.count;typeof e=="undefined"&&(e=this.props.count),t&&this.classList.add(`tonic--theme--${t}`);let i=e>99?"99":String(e),s=["tonic--notifications"];return e>0&&s.push("tonic--new"),this.html`
      <div ... ${{class:s.join(" ")}}>
        <span>${i}</span>
      </div>
    `}};a(nt,"TonicBadge");Ft.exports={TonicBadge:nt}});var Pt=v((fs,Ht)=>{var ii=y(),rt=class extends ii{get value(){return this.props.value}get form(){return this.querySelector("button").form}get disabled(){return this.querySelector("button").hasAttribute("disabled")}set disabled(t){t=String(t),this.props.disabled=t;let e=this.querySelector("button");t==="true"?(e.setAttribute("disabled","true"),this.setAttribute("disabled","true")):(e.removeAttribute("disabled"),this.removeAttribute("disabled"))}defaults(){return{height:this.props.type==="icon"?"100%":"34px",width:this.props.type==="icon"?"100%":"140px",margin:"0px",autofocus:"false",async:!1,radius:"2px",borderWidth:"0px",textColorDisabled:"var(--tonic-disabled)",backgroundColor:"transparent"}}static stylesheet(){return`
      tonic-button {
        display: inline-block;
        user-select: none;
        -webkit-user-select: none;
      }

      tonic-button button {
        color: var(--tonic-button-text, var(--tonic-primary, rgba(54, 57, 61, 1)));
        width: auto;
        font: 12px var(--tonic-subheader, 'Arial', sans-serif);
        font-weight: bold;
        font-size: 12px;
        text-transform: uppercase;
        letter-spacing: 1px;
        padding: 6px;
        position: relative;
        background-color: var(--tonic-button-background, transparent);
        transition: background 0.3s ease, color 0.3s ease;
        box-shadow: 0 1px 2px var(--tonic-button-shadow);
        appearance: none;
      }

      tonic-button[type="icon"] button {
        background: none;
        box-shadow: none;
      }

      tonic-button[type="icon"] tonic-icon {
        position: absolute;
        top: 50%;
        left: 50%;
        transform: translate(-50%, -50%);
      }

      tonic-button button:focus {
        background-color: var(--tonic-button-background-focus, rgba(247, 247, 245, 1));
        outline: none;
      }

      tonic-button button[disabled],
      tonic-button button.tonic--active {
        color: var(--tonic-medium, rgba(153, 157, 160, 1));
        background-color: var(--tonic-button-background-disabled, rgba(247, 247, 245, 1));
      }

      tonic-button button[disabled],
      tonic-button button:active {
        animation-duration: .2s;
        animation-name: tonic--button--activate;
        transition-timing-function: ease;
      }

      @keyframes tonic--button--activate {
        0% {
          transform: scale(1);
        }

        50% {
          transform: scale(0.95);
        }

        100% {
          transform: scale(1);
        }
      }

      tonic-button button:not([disabled]):hover,
      tonic-button:not(.tonic--loading) button:hover {
        background-color: var(--tonic-button-background-hover, rgba(54, 57, 61, 1)) !important;
        cursor: pointer;
      }

      tonic-button.tonic--loading {
        pointer-events: none;
      }

      tonic-button[disabled="true"],
      tonic-button[disabled="true"] button,
      tonic-button button[disabled] {
        pointer-events: none;
        user-select: none;
        background-color: transparent
      }

      tonic-button.tonic--loading button {
        color: transparent !important;
        transition: all 0.3s ease-in;
        pointer-events: none;
      }

      tonic-button.tonic--loading button:hover {
        color: transparent !important;
        background: var(--tonic-medium, rgba(153, 157, 160, 1)) !important;
      }

      tonic-button.tonic--loading button:before {
        margin-top: -8px;
        margin-left: -8px;
        display: inline-block;
        position: absolute;
        top: 50%;
        left: 50%;
        opacity: 1;
        transform: translateX(-50%) translateY(-50%);
        border: 2px solid var(--tonic-button-text, var(--tonic-primary, black));
        border-radius: 50%;
        border-top-color: transparent;
        animation: spin 1s linear 0s infinite;
        transition: opacity 0.3s ease;
      }

      tonic-button button:before {
        content: '';
        width: 14px;
        height: 14px;
        opacity: 0;
      }

      @keyframes spin {
        from {
          transform: rotate(0deg);
        }
        to {
          transform: rotate(360deg);
        }
      }
    `}loading(t){this.classList[t?"add":"remove"]("tonic--loading")}click(t){let e=this.props.disabled==="true",i=this.props.async==="true",s=this.props.href;if(i&&!e&&this.loading(!0),s){let o=this.getAttribute("target");o&&o!=="_self"?window.open(s):window.open(s,"_self")}}styles(){let{width:t,height:e,margin:i,radius:s,borderWidth:o}=this.props;return{button:{width:t,height:e,borderRadius:s,borderWidth:o},wrapper:{width:t,height:e,margin:i}}}render(){let{value:t,type:e,disabled:i,autofocus:s,active:o,async:n,fill:c,tabindex:l,size:h,symbolId:d}=this.props,m=[];o&&m.push("tonic--active"),m=m.join(" "),l&&this.removeAttribute("tabindex");let u="";return e==="icon"?u=this.html`
        <tonic-icon
          size="${h||"18px"}"
          fill="${c||"var(--tonic-primary)"}"
          symbol-id="${d}"
        ></tonic-icon>
      `:u=this.childNodes,this.html`
      <div class="tonic--button--wrapper" styles="wrapper">
        <button ... ${{styles:"button",async:String(n),disabled:i&&i!=="false",autofocus:s==="true"?"autofocus":"",value:t,type:e,tabindex:l,class:m}}>${u}</button>
      </div>
    `}};a(rt,"TonicButton");Ht.exports={TonicButton:rt}});var Bt=v((ms,Tt)=>{var si=y(),at=class extends si{static stylesheet(){return`
      tonic-chart {
        display: inline-block;
        position: relative;
      }

      tonic-chart canvas {
        display: inline-block;
        position: relative;
      }
    `}get library(){return this.Chart}set library(t){this.Chart=t}draw(t={},e={}){let i=this.querySelector("canvas"),s=this.props.type||e.type;if(!this.Chart){console.error("No chart constructor found");return}return new this.Chart(i,{type:s,options:e,data:t})}async redraw(){return this.connected()}async fetch(t,e={}){if(!t)return{};try{return{data:await(await window.fetch(t,e)).json()}}catch(i){return{err:i}}}async connected(){let t=null;if(this.props.library&&(this.Chart=this.props.library),!this.Chart)return;let e={...this.props,...this.props.options},i=this.props.src;if(typeof i=="string"){let s=await this.fetch(i);s.err?(console.error(s.err),t={}):t=s.data}if(i===Object(i)&&(t=i),t&&t.chartData)throw new Error("chartData propery deprecated");t&&this.draw(t,e)}render(){let{width:t,height:e}=this.props;return this.style.width=t,this.style.height=e,this.html`
      <canvas ...${{width:t,height:e}}>
      </canvas>
    `}};a(at,"TonicChart");Tt.exports={TonicChart:at}});var Ut=v((bs,Xt)=>{var oi=y(),ct=class extends oi{constructor(){super();this._modified=!1}get value(){let t=this.state,e=this.props,i=typeof e.checked!="undefined"?e.checked:e.value,s=typeof t.checked!="undefined"?t.checked:t.value,o;return this._modified?o=typeof s!="undefined"?s:i:o=typeof i!="undefined"?i:s,o===!0||o==="true"}set value(t){this._setValue(t)}async _setValue(t){this._modified=!0,this.state._changing=!0;let e=t===!0||t==="true";this.state.checked=e,this.props.checked=e,await this.reRender()}defaults(){return{disabled:!1,size:"18px"}}static stylesheet(){return`
      tonic-checkbox .tonic--checkbox--wrapper {
        display: inline-block;
        -webkit-user-select: none;
        -moz-user-select: none;
        user-select: none;
      }

      tonic-checkbox > div {
        height: auto;
        padding: 6px;
      }

      tonic-checkbox input[type="checkbox"] {
        display: none;
      }

      tonic-checkbox input[type="checkbox"][disabled] + label {
        opacity: 0.35;
      }

      tonic-checkbox label {
        color: var(--tonic-primary, #333);
        font: 12px var(--tonic-subheader, 'Arial', sans-serif);
        font-weight: 500;
        text-transform: uppercase;
        letter-spacing: 1px;
        display: inline;
        vertical-align: middle;
      }

      tonic-checkbox .tonic--icon {
        position: absolute;
        display: inline-block;
        width: 100%;
        height: 100%;
        background-size: contain;
        margin: 4px;
      }

      tonic-checkbox .tonic--icon svg {
        width: inherit;
        height: inherit;
      }

      tonic-checkbox label:nth-of-type(2) {
        display: inline-block;
        line-height: 22px;
        margin: 2px 2px 2px 30px;
      }
    `}change(t){this.props.virtual===!0||this.props.virtual==="true"||this.state._changing||(t.stopPropagation(),this._setValue(!this.value))}updated(){if(this.state._changing){let t=new window.Event("change",{bubbles:!0});this.dispatchEvent(t),delete this.state._changing}}styles(){return{icon:{width:this.props.size,height:this.props.size}}}renderIcon(){let e=this.value?"checked":"unchecked";return this.html`
      <svg>
        <use ... ${{href:`#${e}`,"xlink:href":`#${e}`,color:"var(--tonic-primary, #333)",fill:"var(--tonic-primary, #333)"}}>
        </use>
      </svg>
    `}renderLabel(){let{id:t,label:e}=this.props;return this.props.label||(e=this.nodes),this.html`
      <label
        styles="label"
        for="tonic--checkbox--${t}"
      >${e}</label>
    `}async keydown(t){t.code==="Space"&&(await this._setValue(!this.value),this.querySelector(".tonic--checkbox--wrapper").focus())}render(){let{id:t,disabled:e,theme:i,title:s,tabindex:o}=this.props,n=this.value;return typeof this.state.checked=="undefined"&&(this.state.checked=n),o&&this.removeAttribute("tabindex"),i&&this.classList.add(`tonic--theme--${i}`),this.html`
      <div tabindex="0" class="tonic--checkbox--wrapper">
        <input ... ${{type:"checkbox",id:`tonic--checkbox--${t}`,checked:n,disabled:e===!0||e==="true",title:s,tabindex:o}}/>
        <label
          for="tonic--checkbox--${t}"
          styles="icon"
          class="tonic--icon"
        >
          ${this.renderIcon()}
        </label>
        ${this.renderLabel()}
      </div>
    `}};a(ct,"TonicCheckbox");Xt.exports={TonicCheckbox:ct}});var dt=v((gs,Wt)=>{var X=y(),lt=class extends X{constructor(){super();if(this.classList.add("tonic--dialog"),this.setAttribute("hidden",!0),!document.querySelector(".tonic--dialog--overlay")){let n=document.createElement("div");n.classList="tonic--dialog--overlay",n.textContent=" ",document.body.appendChild(n)}this.closeIcon=document.createElement("div"),this.closeIcon.className="tonic--dialog--close";let t="http://www.w3.org/2000/svg",e="http://www.w3.org/1999/xlink",i=document.createElementNS(t,"svg"),s=document.createElementNS(t,"use");this.closeIcon.appendChild(i),i.appendChild(s),s.setAttributeNS(e,"href","#close"),s.setAttributeNS(e,"xlink:href","#close");let o="var(--tonic-primary, #333)";s.setAttribute("color",o),s.setAttribute("fill",o)}defaults(){return{width:"450px",height:"auto",overlay:!0,backgroundColor:"rgba(0, 0, 0, 0.5)"}}_getZIndex(){return Array.from(document.querySelectorAll("body *")).map(t=>parseFloat(window.getComputedStyle(t).zIndex)).reduce((t,e=Number.MIN_SAFE_INTEGER)=>isNaN(t)||t<e?e:t)}static stylesheet(){return`
      .tonic--dialog {
        box-shadow: 0px 6px 35px 3px rgba(0, 0, 0, 0.2);
        background: var(--tonic-window);
        border: 1px solid var(--tonic-border);
        border-radius: 6px;
        position: absolute;
        overflow: hidden;
        top: 50%;
        left: 50%;
        z-index: -1;
        opacity: 0;
        transition: z-index .25s;
        transform: translate(-50%, -50%) scale(0.88);
      }

      .tonic--dialog.tonic--show {
        transform: translate(-50%, -50%) scale(1);
        opacity: 1;
        animation-duration: .25s;
        animation-name: tonic--dialog--show;
        transition-timing-function: ease;
      }

      .tonic--dialog.tonic--hide {
        transform: translate(-50%, -50%) scale(0.88);
        opacity: 0;
        animation-duration: .2s;
        animation-name: tonic--dialog--hide;
        transition-timing-function: ease;
      }

      .tonic--dialog > .tonic--dialog--close {
        width: 25px;
        height: 25px;
        position: absolute;
        top: 10px;
        right: 10px;
        cursor: pointer;
      }

      .tonic--dialog > .tonic--dialog--close svg {
        width: inherit;
        height: inherit;
      }

      @keyframes tonic--dialog--show {
        from {
          transform: translate(-50%, -50%) scale(0.88);
          opacity: 0;
        }

        to {
          opacity: 1;
          transform: translate(-50%, -50%) scale(1);
        }
      }

      @keyframes tonic--dialog--hide {
        from {
          transform: translate(-50%, -50%) scale(1);
          opacity: 1;
        }

        to {
          opacity: 0;
          transform: translate(-50%, -50%) scale(0.88);
        }
      }

      .tonic--dialog--overlay {
        position: absolute;
        top: 0;
        left: 0;
        right: 0;
        bottom: 0;
        opacity: 0;
        z-index: -1;
        transition: all 0.2s;
        background: var(--tonic-overlay);
      }

      .tonic--dialog--overlay.tonic--show {
        opacity: 1;
      }
    `}show(){let t=this._getZIndex();if(this.appendChild(this.closeIcon),this.removeAttribute("hidden"),this.props.overlay!=="false"){let e=document.querySelector(".tonic--dialog--overlay");e.classList.add("tonic--show"),e.style.zIndex=t}return this.style.zIndex=t+100,new Promise(e=>{this.props.widthMobile&&document.body.clientWidth<500&&(this.props.width=this.props.widthMobile),this.style.width=this.props.width,this.style.height=this.props.height;let i=a(()=>{clearTimeout(s),e()},"done"),s=setTimeout(i,512);this.addEventListener("animationend",i,{once:!0}),this.classList.remove("tonic--hide"),this.classList.add("tonic--show"),this._escapeHandler=o=>{o.keyCode===27&&this.hide()},document.addEventListener("keyup",this._escapeHandler)})}hide(){let t=document.querySelector(".tonic--dialog--overlay");return t.classList.remove("tonic--show"),t.style.zIndex=-1,new Promise(e=>{this.style.zIndex=-1,document.removeEventListener("keyup",this._escapeHandler);let i=a(()=>{clearTimeout(s),this.setAttribute("hidden",!0),e()},"done"),s=setTimeout(i,512);this.addEventListener("animationend",i,{once:!0}),this.classList.remove("tonic--show"),this.classList.add("tonic--hide")})}event(t){let e=this;return{then(i){let s=a(n=>{n.keyCode===27&&i({})},"resolver"),o=a(n=>{let c=X.match(n.target,".tonic--dialog--close"),l=X.match(n.target,"[value]");if((c||l)&&(e.removeEventListener(t,o),document.removeEventListener("keyup",s)),c)return i({});l&&i({[n.target.value]:!0})},"listener");document.addEventListener("keyup",s),e.addEventListener(t,o)}}}click(t){X.match(t.target,".tonic--dialog--close")&&this.hide()}updated(){this.appendChild(this.closeIcon)}render(){return this.html`
      ${this.children}
    `}};a(lt,"TonicDialog");Wt.exports={TonicDialog:lt}});var Jt=v((vs,Zt)=>{var Gt=y(),Y={type:"TonicForm_NON_EXISTANT"},_=class extends Gt{static isNumber(t){return!isNaN(Number(t))}static getPropertyValue(t,e){if(!e)return Y;let i=e.split("."),s=t;for(let o of i){if(!s)return Y;s=s[o]}return s}static setPropertyValue(t,e,i){if(!e)return;let s=e.split("."),o=t,n=s.pop();if(!!n){for(let c=0;c<s.length;c++){let l=s[c],h=s[c+1]||n;o[l]||(o[l]=_.isNumber(h)?[]:{}),o=o[l]}return o[n]=i,t}}setData(t){this.state=t}getData(){return this.value}getElements(){return[...this.querySelectorAll("[data-key]")]}get value(){let t=this.getElements();for(let e of t)_.setPropertyValue(this.state,e.dataset.key,e.value);return this.state}reset(t={}){let e=this.getElements();this.state=t;for(let i of e)i.value="",i.state.edited&&(i.state.edited=!1,i.removeAttribute("edited"));return this.state}set value(t){if(typeof t!="object")return;let e=this.getElements();for(let i of e){let s=_.getPropertyValue(t,i.dataset.key);s!==Y&&(i.value=s||"")}this.state=t}setValid(){let t=this.getElements();for(let e of t)e.setValid&&(e.state.edited=!1,e.removeAttribute("edited"),e.setValid())}input(t){this.change(t)}change(t){let e=Gt.match(t.target,"[data-key]");!e||_.setPropertyValue(this.state,e.dataset.key,e.value)}validate({decorate:t=!0}={}){this.getData();let e=this.getElements(),i=!0;for(let s of e){if(!s.setInvalid)continue;let o="";s.tagName==="TONIC-INPUT"?o="input":s.tagName==="TONIC-TEXTAREA"?o="textarea":s.tagName==="TONIC-SELECT"?o="select":(s.tagName==="TONIC-CHECKBOX"||s.tagName==="TONIC-TOGGLE")&&(o="checkbox");let n=o?s.querySelector(o):s;if(!!n.checkValidity){n.checkValidity();for(let c in n.validity){if(c==="valid"||c==="customError"||!n.validity[c]){t&&s.setValid();continue}t&&s.setInvalid(s.props.errorMessage||"Required"),i=!1}}}return i}connected(){if(!this.props.fill)return;let t=this.getElements();for(let e of t){let i=e.dataset.key,s=_.getPropertyValue(this.state,i);s===Y&&(s=""),e.value=s||e.value||""}}render(){return this.html`
      ${this.childNodes}
    `}};a(_,"TonicForm");_.NON_EXISTANT=Y;Zt.exports={TonicForm:_}});var Qt=v((ys,Kt)=>{var ni=y(),pt=class extends ni{defaults(){return{size:"25px",fill:"var(--tonic-primary, #333)"}}static stylesheet(){return`
      tonic-icon {
        vertical-align: middle;
      }

      tonic-icon svg path {
        fill: inherit;
      }
    `}styles(){let{size:t}=this.props;return{icon:{width:t,height:t}}}render(){let{symbolId:t,size:e,fill:i,theme:s,src:o,tabindex:n}=this.props;return n&&this.removeAttribute("tabindex"),s&&this.classList.add(`tonic--theme--${s}`),this.html`
      <svg ... ${{styles:"icon",tabindex:n}}>
        <use ... ${{href:`${o||""}#${t}`,"xlink:href":`${o||""}#${t}`,width:e,fill:i,color:i,height:e}}>
      </svg>
    `}};a(pt,"TonicIcon");Kt.exports={TonicIcon:pt}});var ee=v((xs,te)=>{var ri=y(),ut=class extends ri{constructor(){super();this._modified=!1}defaults(){return{type:"text",placeholder:"",color:"var(--tonic-primary)",spellcheck:!1,ariaInvalid:!1,invalid:!1,radius:"3px",disabled:!1,position:"left"}}get form(){return this.querySelector("input").form}get value(){return this._modified?typeof this.state.value=="string"?this.state.value:this.props.value:typeof this.props.value=="string"?this.props.value:this.state.value}set value(t){this._modified=!0,this.querySelector("input").value=t,this.state.value=t}setValid(){let t=this.querySelector("input");!t||(t.setCustomValidity(""),t.removeAttribute("invalid"))}setInvalid(t){let e=this.querySelector("input");if(!e)return;this.setAttribute("edited",!0),this.state.edited=!0,t=t||this.props.errorMessage,e.setCustomValidity(t),window.requestAnimationFrame(()=>{e.setAttribute("invalid",t)});let i=this.querySelector(".tonic--invalid span");if(!i)return;i.textContent=t;let s=this.querySelector(".tonic--invalid");s.style.display="block"}static stylesheet(){return`
      tonic-input .tonic--wrapper {
        position: relative;
      }

      tonic-input[symbol-id] .tonic--right tonic-icon,
      tonic-input[src] .tonic--right tonic-icon {
        right: 6px;
      }

      tonic-input[symbol-id] .tonic--right input,
      tonic-input[src] .tonic--right input {
        padding-right: 40px;
      }

      tonic-input[symbol-id] .tonic--left tonic-icon,
      tonic-input[src] .tonic--left tonic-icon {
        left: 6px;
      }

      tonic-input[symbol-id] .tonic--left input,
      tonic-input[src] .tonic--left input {
        padding-left: 40px;
      }

      tonic-input[symbol-id] tonic-icon,
      tonic-input[src] tonic-icon {
        position: absolute;
        bottom: 10px;
      }

      tonic-input label {
        color: var(--tonic-medium, #999);
        font-weight: 500;
        font: 12px/14px var(--tonic-subheader, 'Arial', sans-serif);
        text-transform: uppercase;
        letter-spacing: 1px;
        padding-bottom: 10px;
        display: block;
        user-select: none;
        -webkit-user-select: none;
      }

      tonic-input input {
        color: var(--tonic-primary, #333);
        font: 14px var(--tonic-monospace, 'Andale Mono', monospace);
        padding: 8px;
        background-color: var(--tonic-input-background, var(--tonic-background, transparent));
        border: 1px solid var(--tonic-border, #ccc);
        -webkit-appearance: none;
        -moz-appearance: none;
        appearance: none;
      }

      tonic-input input:focus {
        background-color: var(--tonic-input-background-focus, rgba(241, 241, 241, 0.75));
        outline: none;
      }

      tonic-input[edited] input[invalid]:focus,
      tonic-input[edited] input:invalid:focus,
      tonic-input[edited] input[invalid],
      tonic-input[edited] input:invalid {
        border-color: var(--tonic-error, #f66);
      }

      tonic-input[edited] input[invalid] ~ .tonic--invalid,
      tonic-input[edited] input:invalid ~ .tonic--invalid {
        transform: translateY(0);
        visibility: visible;
        opacity: 1;
        transition: opacity 0.2s ease, transform 0.2s ease, visibility 1s ease 0s;
      }

      tonic-input input[disabled] {
        background-color: transparent;
      }

      tonic-input[label] .tonic--invalid {
        margin-bottom: -13px;
      }

      tonic-input .tonic--invalid {
        font-size: 14px;
        text-align: center;
        margin-bottom: 13px;
        position: absolute;
        bottom: 100%;
        left: 0;
        right: 0;
        transform: translateY(-10px);
        transition: opacity 0.2s ease, transform 0.2s ease, visibility 0s ease 1s;
        visibility: hidden;
        opacity: 0;
        z-index: 1;
      }

      tonic-input .tonic--invalid span {
        color: white;
        padding: 10px;
        background-color: var(--tonic-error, #f66);
        border-radius: 2px;
        position: relative;
        display: inline-block;
        margin: 0 auto;
      }

      tonic-input .tonic--invalid span:after {
        content: '';
        width: 0;
        height: 0;
        display: block;
        position: absolute;
        bottom: -6px;
        left: 50%;
        transform: translateX(-50%);
        border-left: 6px solid transparent;
        border-right: 6px solid transparent;
        border-top: 6px solid var(--tonic-error, #f66);
      }
    `}renderLabel(){return this.props.label?this.html`
      <label
        for="tonic--input_${this.props.id}"
      >${this.props.label}</label>
    `:""}renderIcon(){if(!this.props.src&&!this.props.symbolId)return"";let t={};return this.props.src?t.src=this.props.src:this.props.symbolId&&(t.symbolId=this.props.symbolId),this.props.color&&(t.color=this.props.color),this.props.fill&&(t.fill=this.props.fill),t.size="20px",this.html`
      <tonic-icon ...${t}>
      </tonic-icon>
    `}setFocus(t){let e=this.querySelector("input");e.focus();try{e.setSelectionRange(t,t)}catch(i){console.warn(i)}}setupEvents(){let t=this.querySelector("input"),e=a(s=>{this.dispatchEvent(new window.CustomEvent(s,{bubbles:!0}))},"relay");t.addEventListener("focus",s=>{this.state.focus=!0,e("focus")}),t.addEventListener("blur",s=>{this.state.rerendering||(this.state.focus=!1,e("blur"))}),t.addEventListener("input",s=>{this._modified=!0,this.state.edited=!0,this.setAttribute("edited",!0),this.state.value=s.target.value,this.state.pos=s.target.selectionStart});let i=this.state;!i.focus||this.setFocus(i.pos)}updated(){this.setupEvents(),setTimeout(()=>{this.props.invalid?this.setInvalid(this.props.errorMessage):this.setValid()},32),this.state.rerendering=!1}async reRender(...t){return this.state.rerendering=!0,super.reRender(...t)}connected(){this.updated()}styles(){let{width:t,height:e,radius:i,padding:s}=this.props;return{wrapper:{width:t},input:{width:"100%",height:e,borderRadius:i,padding:s}}}render(){let{ariaInvalid:t,ariaLabelledby:e,disabled:i,height:s,label:o,max:n,maxlength:c,min:l,minlength:h,name:d,pattern:m,placeholder:u,position:b,readonly:f,required:x,spellcheck:k,tabindex:g,theme:E,title:N,type:$}=this.props;e&&this.removeAttribute("ariaLabelledby"),s&&(this.style.height=s),d&&this.removeAttribute("name"),g&&this.removeAttribute("tabindex"),E&&this.classList.add(`tonic--theme--${E}`);let q=this.value,D=this.props.errorMessage||this.props.errorMessage||"Invalid",A=["tonic--wrapper"];b&&A.push(`tonic--${b}`);let L=this.elements.length?this.props.id+"_datalist":null,C={ariaInvalid:t,ariaLabel:o,"aria-labelledby":e,disabled:i==="true",max:n,maxlength:c,min:l,minlength:h,name:d,pattern:m,placeholder:u,position:b,readonly:f==="true",required:x==="true",spellcheck:k,tabindex:g,title:N,value:q,list:L};this.state.edited&&this.setAttribute("edited",!0);let O="";return L&&(O=this.html`
        <datalist id="${L}">
          ${this.elements}
        </datalist>
      `),this.html`
      <div class="${A.join(" ")}" styles="wrapper">
        ${this.renderLabel()}
        ${this.renderIcon()}

        <input ... ${{styles:"input",type:$,id:`tonic--input_${this.props.id}`,...C}}/>

        <div class="tonic--invalid">
          <span id="tonic--error-${this.props.id}">${D}</span>
        </div>

        ${O}
      </div>
    `}};a(ut,"TonicInput");te.exports={TonicInput:ut}});var se=v((ws,ie)=>{var ai=y(),ht=class extends ai{constructor(){super();this.attachShadow({mode:"open"}),this.setAttribute("id",Math.random().toString(16).slice(2,-8))}stylesheet(){return`
      .outer {
        position: absolute;
        top: 0;
        bottom: 0;
        left: 0;
        right: 0;
      }

      .inner {
        position: absolute;
        top: 50%;
        left: 50%;
        transform: translate(-50%, -50%);
      }
    `}render(){let t=this.props.timeout||"128";return t==="-1"&&(t="indefinite"),this.html`
      <div class="outer">
        <div class="inner">
          <svg version="1.1" id="loader-1" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" x="0px" y="0px"
           width="40px" height="40px" viewBox="0 0 40 40" enable-background="new 0 0 40 40" xml:space="preserve">
          <path opacity="0.2" fill="#000" d="M20.201,5.169c-8.254,0-14.946,6.692-14.946,14.946c0,8.255,6.692,14.946,14.946,14.946
            s14.946-6.691,14.946-14.946C35.146,11.861,28.455,5.169,20.201,5.169z M20.201,31.749c-6.425,0-11.634-5.208-11.634-11.634
            c0-6.425,5.209-11.634,11.634-11.634c6.425,0,11.633,5.209,11.633,11.634C31.834,26.541,26.626,31.749,20.201,31.749z"/>
          <path fill="#000" d="M26.013,10.047l1.654-2.866c-2.198-1.272-4.743-2.012-7.466-2.012h0v3.312h0
            C22.32,8.481,24.301,9.057,26.013,10.047z">
            <animateTransform attributeType="xml"
              attributeName="transform"
              type="rotate"
              from="0 20 20"
              to="360 20 20"
              dur="0.8s"
              repeatCount="${t}"/>
            </path>
          </svg>
        </div>
      </div>
    `}};a(ht,"TonicLoader");ie.exports={TonicLoader:ht}});var ne=v((ks,oe)=>{var{TonicDialog:ci}=dt(),ft=class extends ci{constructor(){super();this.classList.add("tonic--panel")}defaults(){return{position:"right"}}stylesheet(){let{width:t,position:e}=this.props,i=[0,t];return e!=="right"&&(i[1]=`-${t}`),`
      .tonic--dialog.tonic--panel,
      .tonic--dialog.tonic--show.tonic--panel {
        left: unset;
        border-radius: 0;
        top: 0;
        width: ${t};
        bottom: 0;
        ${e}: 0;
        transform: translateX(${i[0]});
        animation-name: tonic--panel--show;
      }

      .tonic--dialog.tonic--hide.tonic--panel {
        opacity: 0;
        transform: translateX(${t});
        animation-name: tonic--panel--hide;
      }

      @keyframes tonic--panel--show {
        from {
          opacity: 0;
          transform: translateX(${i[1]});
        }

        to {
          opacity: 1;
          transform: translateX(${i[0]});
        }
      }

      @keyframes tonic--panel--hide {
        from {
          opacity: 1;
          transform: translateX(${i[0]});
        }

        to {
          opacity: 0;
          transform: translateX(${i[1]});
        }
      }
    `}};a(ft,"TonicPanel");oe.exports={TonicPanel:ft}});var ce=v((Es,ae)=>{var re=y(),mt=class extends re{constructor(){super();let t=this.getAttribute("for"),e=document.getElementById(t);e.addEventListener("click",i=>this.show(e))}defaults(t){return{width:"auto",height:"auto",padding:"15px",margin:10,position:"bottomleft"}}static stylesheet(){return`
      tonic-popover .tonic--overlay {
        position: fixed;
        top: 0;
        left: 0;
        right: 0;
        bottom: 0;
        opacity: 0;
        display: none;
        z-index: 0;
        background-color: rgba(0,0,0,0);
      }

      tonic-popover .tonic--popover {
        position: absolute;
        top: 30px;
        background: var(--tonic-window, #fff);
        border: 1px solid var(--tonic-border, #ccc);
        border-radius: 2px;
        visibility: hidden;
        z-index: -1;
        opacity: 0;
        -webkit-transform: scale(0.75);
        ms-transform: scale(0.75);
        transform: scale(0.75);
        transition: transform 0.1s ease-in-out, opacity 0s ease 0.1s, visibility 0s ease 0.1s, z-index 0s ease 0.1s;
      }

      tonic-popover .tonic--popover.tonic--show {
        box-shadow: 0px 30px 90px -20px rgba(0, 0, 0, 0.3);
        -webkit-transform: scale(1);
        -ms-transform: scale(1);
        transform: scale(1);
        visibility: visible;
        transition: transform 0.15s ease-in-out;
        opacity: 1;
        z-index: 1;
      }

      tonic-popover .tonic--show ~ .tonic--overlay {
        display: block;
        opacity: 1;
      }

      tonic-popover .tonic--popover--top {
        transform-origin: bottom center;
      }

      tonic-popover .tonic--popover--topleft {
        transform-origin: bottom left;
      }

      tonic-popover .tonic--popover--topright {
        transform-origin: bottom right;
      }

      tonic-popover .tonic--popover--bottom {
        transform-origin: top center;
      }

      tonic-popover .tonic--popover--bottomleft {
        transform-origin: top left;
      }

      tonic-popover .tonic--popover--bottomright {
        transform-origin: top right;
      }

    `}styles(){let{width:t,height:e,padding:i,margin:s,position:o}=this.props;return{popover:{width:t,height:e,padding:i,margin:s,position:o}}}show(t){let e=this.querySelector(".tonic--popover"),i=t.parentNode;for(;!(!i||i.tagName==="BODY"||window.getComputedStyle(i).overflow==="scroll");)i=i.parentNode;let s=parseInt(this.props.margin,10),{top:o,left:n}=t.getBoundingClientRect(),c=o+i.scrollTop;switch(n-=i.offsetLeft,this.props.position){case"topleft":c-=e.offsetHeight+s;break;case"topright":c-=e.offsetHeight+s,n+=t.offsetWidth-e.offsetWidth;break;case"top":c-=e.offsetHeight+s,n+=t.offsetWidth/2-e.offsetWidth/2;break;case"bottomleft":c+=t.offsetHeight+s;break;case"bottomright":c+=t.offsetHeight+s,n+=t.offsetWidth-e.offsetWidth;break;case"bottom":c+=t.offsetHeight+s,n+=t.offsetWidth/2-e.offsetWidth/2;break}e.style.top=`${c}px`,e.style.left=`${n}px`,window.requestAnimationFrame(()=>{e.className=`tonic--popover tonic--show tonic--popover--${this.props.position}`;let l=new window.Event("show");this.dispatchEvent(l)})}hide(){let t=this.querySelector(".tonic--popover");t&&t.classList.remove("tonic--show")}connected(){if(!this.props.open)return;let t=this.getAttribute("for");this.show(document.getElementById(t))}click(t){if(re.match(t.target,".tonic--overlay"))return this.hide()}render(){let{theme:t}=this.props;return t&&this.classList.add(`tonic--theme--${t}`),this.html`
      <div class="tonic--popover" styles="popover">
        ${this.nodes}
      </div>
      <div class="tonic--overlay"></div>
    `}};a(mt,"TonicPopover");ae.exports={TonicPopover:mt}});var de=v((As,le)=>{var li=y(),z=class extends li{get value(){return this.state.data||this.props.src}defaults(){return{src:z.svg.default(),size:"50px",radius:"5px"}}static stylesheet(){return`
      tonic-profile-image {
        display: inline-block;
      }

      tonic-profile-image .tonic--wrapper {
        position: relative;
        overflow: hidden;
      }

      tonic-profile-image .tonic--image {
        position: absolute;
        top: 0;
        left: 0;
        right: 0;
        bottom: 0;
        background-size: cover;
        background-position: center center;
        background-repeat: no-repeat;
      }

      tonic-profile-image .tonic--overlay {
        position: absolute;
        top: 0;
        left: 0;
        right: 0;
        bottom: 0;
        background-color: rgba(0,0,0,0.5);
        transition: opacity 0.2s ease-in-out;
        visibility: hidden;
        opacity: 0;
        display: flex;
      }

      tonic-profile-image .tonic--overlay .tonic--icon {
        width: 30px;
        height: 30px;
        position: absolute;
        top: 50%;
        left: 50%;
        -webkit-transform: translateX(-50%) translateY(-50%);
        -moz-transform: translateX(-50%) translateY(-50%);
        transform: translateX(-50%) translateY(-50%);
      }

      tonic-profile-image .tonic--overlay .tonic--icon svg {
        width: inherit;
        height: inherit;
      }

      tonic-profile-image .tonic--wrapper.tonic--editable:hover .tonic--overlay {
        visibility: visible;
        opacity: 1;
        cursor: pointer;
      }
    `}styles(){let{src:t,size:e,border:i,radius:s}=this.props;return{background:{backgroundImage:`url('${t}')`},hidden:{display:"none"},wrapper:{width:e,height:e,border:i,borderRadius:s}}}getPictureData(t,e){let i=new window.FileReader;i.onerror=s=>e(s),i.onload=s=>e(null,s.target.result),i.readAsDataURL(t)}click(t){if(this.props.editable){if(this.props.editable==="false")return;this.getElementsByTagName("input")[0].click()}}change(t){let i=this.getElementsByTagName("input")[0].files[0];if(t.data)return;t.stopPropagation();let{size:s,type:o,path:n,lastModifiedDate:c}=i;this.getPictureData(i,(l,h)=>{if(l){let u=new window.Event("error");u.message=l.message,this.dispatchEvent(u);return}let d=this.querySelector(".tonic--image");this.state.size=s,this.state.path=n,this.state.time=o,this.state.mtime=c,this.state.data=h,d.style.backgroundImage='url("'+h+'")';let m=new window.Event("change",{bubbles:!0});m.data=!0,this.dispatchEvent(m)})}render(){let{theme:t,editable:e}=this.props;t&&this.classList.add(`tonic--theme--${t}`);let i=["tonic--wrapper"];return e==="true"&&i.push("tonic--editable"),this.html`
      <div
        class="${i.join(" ")}"
        styles="wrapper">

        <div
          class="tonic--image"
          styles="background">
        </div>

        <input type="file" styles="hidden"/>

        <div class="tonic--overlay">
          <div class="tonic--icon">
            <svg>
              <use
                href="#edit"
                xlink:href="#edit"
                color="#fff"
                fill="#fff">
              </use>
            </svg>
          </div>
        </div>
      </div>
    `}};a(z,"TonicProfileImage");z.svg={};z.svg.toURL=r=>`data:image/svg+xml;base64,${window.btoa(r)}`;z.svg.default=()=>z.svg.toURL(`
  <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
    <rect fill="#F0F0F0" width="100" height="100"></rect>
    <circle fill="#D6D6D6" cx="49.3" cy="41.3" r="21.1"></circle>
    <path fill="#D6D6D6" d="M48.6,69.5c-18.1,0-33.1,13.2-36,30.5h72C81.8,82.7,66.7,69.5,48.6,69.5z"></path>
  </svg>
`);le.exports={TonicProfileImage:z}});var ue=v(($s,pe)=>{var di=y(),bt=class extends di{set value(t){this.setProgress(t)}get value(){return typeof this.state.progress!="undefined"?this.state.progress:this.props.progress}defaults(){return{width:"280px",height:"15px",progress:0}}static stylesheet(){return`
      tonic-progress-bar {
        display: inline-block;
        user-select: none;
        -webkit-user-select: none;
      }

      tonic-progress-bar .tonic--wrapper {
        position: relative;
        background-color: var(--tonic-background, #fff);
      }

      tonic-progress-bar .tonic--wrapper .tonic--progress {
        background-color: var(--tonic-accent, #f66);
        width: 0%;
        height: 100%;
      }
    `}styles(){let t=this.state.progress||this.props.progress;return{wrapper:{width:this.props.width,height:this.props.height},progress:{width:t+"%",backgroundColor:this.props.color||"var(--tonic-accent, #f66)"}}}setProgress(t){this.state.progress=t,this.reRender()}render(){return this.props.theme&&this.classList.add(`tonic--theme--${this.props.theme}`),this.style.width=this.props.width,this.style.height=this.props.height,this.html`
      <div class="tonic--wrapper" styles="wrapper">
        <div class="tonic--progress" styles="progress"></div>
      </div>
    `}};a(bt,"TonicProgressBar");pe.exports={TonicProgressBar:bt}});var fe=v((Ss,he)=>{var pi=y(),gt=class extends pi{defaults(){return{width:"250px",disabled:!1,min:"0",max:"100",value:"50"}}get value(){return this.state.value}set value(t){this.querySelector("input").value=t,this.setValue(t)}setValue(t){let e=this.props.min,i=this.props.max,s=this.querySelector("input");if(this.props.label){let o=this.querySelector("label");o.textContent=this.getLabelValue(t)}s.style.backgroundSize=(t-e)*100/(i-e)+"% 100%",this.state.value=t}input(t){this.setValue(t.target.value||this.props.value)}static stylesheet(){return`
      tonic-range  {
        position: relative;
        display: -webkit-flex;
        display: flex;
        height: 50px;
        padding: 20px 0;
      }

      tonic-range.tonic--no-label {
        height: 30px;
      }

      tonic-range label {
        font: 13px var(--tonic-subheader, 'Arial', sans-serif);
        letter-spacing: 1px;
        text-align: center;
        position: absolute;
        top: 0;
        left: 0;
        right: 0;
      }

      tonic-range input[type="range"] {
        margin: auto;
        padding: 0;
        width: 50%;
        height: 4px;
        background-color: var(--tonic-secondary, #fff);
        background-image: -webkit-gradient(linear, 50% 0%, 50% 100%, color-stop(0%, var(--tonic-accent, #f66)), color-stop(100%, var(--tonic-accent, #f66)));
        background-image: -webkit-linear-gradient(var(--tonic-accent, #f66), var(--tonic-accent, #f66));
        background-image: -moz-linear-gradient(var(--tonic-accent, #f66), var(--tonic-accent, #f66));
        background-image: -o-linear-gradient(var(--tonic-accent, #f66), var(--tonic-accent, #f66));
        background-image: linear-gradient(var(--tonic-accent, #f66), var(--tonic-accent, #f66));
        background-size: 50% 100%;
        background-repeat: no-repeat;
        border-radius: 0;
        cursor: pointer;
        -webkit-appearance: none;
      }

      tonic-range input[type="range"]:focus {
        outline-offset: 4px;
      }

      tonic-range input[type="range"]:disabled {
        background-image: -webkit-gradient(linear, 50% 0%, 50% 100%, color-stop(0%, var(--tonic-border, #ccc)), color-stop(100%, var(--tonic-border, #ccc)));
        background-image: -webkit-linear-gradient(var(--tonic-border, #ccc), var(--tonic-border, #ccc));
        background-image: -moz-linear-gradient(var(--tonic-border, #ccc), var(--tonic-border, #ccc));
        background-image: -o-linear-gradient(var(--tonic-border, #ccc), var(--tonic-border, #ccc));
        background-image: linear-gradient(var(--tonic-border, #ccc), var(--tonic-border, #ccc));
      }

      tonic-range input[type="range"]::-webkit-slider-runnable-track {
        box-shadow: none;
        border: none;
        background: transparent;
        -webkit-appearance: none;
      }

      tonic-range input[type="range"]::-moz-range-track {
        box-shadow: none;
        border: none;
        background: transparent;
      }

      tonic-range input[type="range"]::-moz-focus-outer {
        border: 0;
      }

      tonic-range input[type="range"]::-webkit-slider-thumb {
        width: 18px;
        height: 18px;
        border: 0;
        background: var(--tonic-border);
        border-radius: 100%;
        box-shadow: 0 0 3px 0px rgba(0,0,0,0.25);
        -webkit-appearance: none;
      }

      tonic-range input[type="range"]::-moz-range-thumb {
        width: 18px;
        height: 18px;
        border: 0;
        background: var(--tonic-border);
        border-radius: 100%;
        box-shadow: 0 0 1px 0px rgba(0,0,0,0.1);
      }
    `}getLabelValue(t){return this.setLabel?this.setLabel(t):this.props.label?this.props.label.replace(/%\w/,t):t}renderLabel(){if(!this.props.label)return"";let t=this.props.value;return this.html`<label>${this.getLabelValue(t)}</label>`}styles(){let{width:t}=this.props;return{width:{width:t}}}connected(){this.setValue(this.state.value)}render(){let{width:t,height:e,disabled:i,theme:s,min:o,max:n,step:c,label:l,id:h,tabindex:d}=this.props;t&&(this.style.width=t),e&&(this.style.width=e),s&&this.classList.add(`tonic--theme--${s}`),l||this.classList.add("tonic--no-label"),d&&this.removeAttribute("tabindex");let m=this.props.value||this.state.value;return typeof this.state.value=="undefined"&&(this.state.value=m),this.html`
      ${this.renderLabel()}
      <div class="tonic--wrapper" styles="width">
        <input ... ${{type:"range",styles:"width",id:h,value:m,tabindex:d,step:c,min:o,max:n,disabled:i==="true"}}/>
      </div>
    `}};a(gt,"TonicRange");he.exports={TonicRange:gt}});var ve=v((qs,ge)=>{var ui=y(),me=["Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"],be=["January","February","March","April","May","June","July","August","September","October","November","December"];function V(r){return`0${r}`.slice(-2)}a(V,"pad");function vt(r,t){let e=r.getDay(),i=r.getDate(),s=r.getMonth(),o=r.getFullYear(),n=r.getHours(),c=r.getMinutes(),l=r.getSeconds();return t.replace(/%([%aAbBcdeHIlmMpPSwyYZz])/g,h=>{let d;switch(h[1]){case"%":return"%";case"a":return me[e].slice(0,3);case"A":return me[e];case"b":return be[s].slice(0,3);case"B":return be[s];case"c":return r.toString();case"d":return V(i);case"e":return String(i);case"H":return V(n);case"I":return V(vt(r,"%l"));case"l":return String(n===0||n===12?12:(n+12)%12);case"m":return V(s+1);case"M":return V(c);case"p":return n>11?"PM":"AM";case"P":return n>11?"pm":"am";case"S":return V(l);case"w":return String(e);case"y":return V(o%100);case"Y":return String(o);case"Z":return d=r.toString().match(/\((\w+)\)$/),d?d[1]:"";case"z":return d=r.toString().match(/\w([+-]\d\d\d\d) /),d?d[1]:""}return""})}a(vt,"strftime");function yt(r){let t;return function(){if(t)return t;if("Intl"in window)try{return t=new Intl.DateTimeFormat(void 0,r),t}catch(e){if(!(e instanceof RangeError))throw e}}}a(yt,"makeFormatter");var U=null,hi=yt({day:"numeric",month:"short"});function fi(){if(U!==null)return U;let r=hi();return r?(U=!!r.format(new Date(0)).match(/^\d/),U):!1}a(fi,"isDayFirst");var W=null,mi=yt({day:"numeric",month:"short",year:"numeric"});function bi(){if(W!==null)return W;let r=mi();return r?(W=!!r.format(new Date(0)).match(/\d,/),W):!0}a(bi,"isYearSeparator");function gi(r){return new Date().getUTCFullYear()===r.getUTCFullYear()}a(gi,"isThisYear");function vi(r,t){if("Intl"in window&&"RelativeTimeFormat"in window.Intl)try{return new Intl.RelativeTimeFormat(r,t)}catch(e){if(!(e instanceof RangeError))throw e}}a(vi,"makeRelativeFormat");function yi(r){let t=r.closest("[lang]");return t instanceof window.HTMLElement&&t.lang?t.lang:"default"}a(yi,"localeFromElement");var G=class{constructor(t,e){this.date=t,this.locale=e}toString(){let t=this.timeElapsed();if(t)return t;let e=this.timeAhead();return e||`on ${this.formatDate()}`}get value(){return this.date}timeElapsed({date:t=this.date,locale:e=this.locale}={}){let i=new Date().getTime()-t.getTime(),s=Math.round(i/1e3),o=Math.round(s/60),n=Math.round(o/60),c=Math.round(n/24);return i>=0&&c<30?this.timeAgoFromMs({ms:i,date:t,locale:e}):null}timeAhead({date:t=this.date,locale:e=this.locale}={}){let i=t.getTime()-new Date().getTime(),s=Math.round(i/1e3),o=Math.round(s/60),n=Math.round(o/60),c=Math.round(n/24);return i>=0&&c<30?this.timeUntil({date:t,locale:e}):null}timeAgo({date:t=this.date,locale:e=this.locale}={}){let i=new Date().getTime()-t.getTime();return this.timeAgoFromMs({ms:i,date:t,locale:e})}timeAgoFromMs({ms:t,locale:e=this.locale}={}){let i=Math.round(t/1e3),s=Math.round(i/60),o=Math.round(s/60),n=Math.round(o/24),c=Math.round(n/30),l=Math.round(c/12);return t<0||i<10?w(e,0,"second"):i<45?w(e,-i,"second"):i<90||s<45?w(e,-s,"minute"):s<90||o<24?w(e,-o,"hour"):o<36||n<30?w(e,-n,"day"):c<12?w(e,-c,"month"):(c<18,w(e,-l,"year"))}timeUntil({date:t=this.date,locale:e=this.locale}={}){let i=t.getTime()-new Date().getTime();return this.timeUntilFromMs({ms:i,locale:e})}timeUntilFromMs({ms:t,locale:e=this.locale}={}){let i=Math.round(t/1e3),s=Math.round(i/60),o=Math.round(s/60),n=Math.round(o/24),c=Math.round(n/30),l=Math.round(c/12);return c>=18||c>=12?w(e,l,"year"):n>=45||n>=30?w(e,c,"month"):o>=36||o>=24?w(e,n,"day"):s>=90||s>=45?w(e,o,"hour"):i>=90||i>=45?w(e,s,"minute"):i>=10?w(e,i,"second"):w(e,0,"second")}formatDate({date:t=this.date}={}){let e=fi()?"%e %b":"%b %e";return gi(t)||(e+=bi()?", %Y":" %Y"),vt(t,e)}formatTime({date:t=this.date}={}){let e=wi();return e?e.format(t):vt(t,"%l:%M%P")}};a(G,"RelativeTime");function w(r,t,e){let i=vi(r,{numeric:"auto"});return i?i.format(t,e):xi(t,e)}a(w,"formatRelativeTime");function xi(r,t){if(r===0)switch(t){case"year":case"quarter":case"month":case"week":return`this ${t}`;case"day":return"today";case"hour":case"minute":return`in 0 ${t}s`;case"second":return"now"}else if(r===1)switch(t){case"year":case"quarter":case"month":case"week":return`next ${t}`;case"day":return"tomorrow";case"hour":case"minute":case"second":return`in 1 ${t}`}else if(r===-1)switch(t){case"year":case"quarter":case"month":case"week":return`last ${t}`;case"day":return"yesterday";case"hour":case"minute":case"second":return`1 ${t} ago`}else if(r>1)switch(t){case"year":case"quarter":case"month":case"week":case"day":case"hour":case"minute":case"second":return`in ${r} ${t}s`}else if(r<-1)switch(t){case"year":case"quarter":case"month":case"week":case"day":case"hour":case"minute":case"second":return`${-r} ${t}s ago`}throw new RangeError(`Invalid unit argument for format() '${t}'`)}a(xi,"formatEnRelativeTime");var xt=class extends ui{render(){let t=this.props.date||"",e=this.props.locale||yi(this);return typeof t=="string"&&(t=this.props.date=new Date(this.props.date)),t.toString()==="Invalid Date"&&(t=new Date),this.props.refresh&&(this.interval=setInterval(()=>{this.reRender(s=>({...s,date:t}))},+this.props.refresh)),new G(t,e).toString()}};a(xt,"TonicRelativeTime");var wi=yt({hour:"numeric",minute:"2-digit"});ge.exports={TonicRelativeTime:xt,RelativeTime:G}});var xe=v((Ls,ye)=>{var ki=y(),S=class extends ki{constructor(){super();if(S.patched)return;S.patched=!0;let t=a(function(e){let i=window.history[e];return function(...s){let o=i.call(this,...s);return window.dispatchEvent(new window.Event(e.toLowerCase())),S.route(),o}},"patchEvent");window.addEventListener("popstate",e=>S.route()),window.history.pushState=t("pushState"),window.history.replaceState=t("replaceState")}static route(t,e){t=t||[...document.getElementsByTagName("tonic-router")];let i=[],s=null,o=!1;S.props={};for(let n of t){let c=n.getAttribute("path");if(n.removeAttribute("match"),!c){s=n,s.reRender&&s.reRender();continue}let h=S.matcher(c,i).exec(window.location.pathname);h?(n.setAttribute("match",!0),o=!0,h.slice(1).forEach((d,m)=>{S.props[i[m].name]=d})):n.removeAttribute("match"),e||n.reRender&&n.reRender()}!e&&!o&&s&&(s.setAttribute("match",!0),s.reRender&&s.reRender())}willConnect(){let t=this.getAttribute("id");this.id=t||this.getAttribute("path"),this.template=document.createElement("template"),this.template.innerHTML=this.innerHTML,S.route([this],!0)}updated(){this.hasAttribute("match")&&this.dispatchEvent(new window.Event("match"))}render(){return this.hasAttribute("match")?(this.state=S.props,this.template.content):""}};a(S,"TonicRouter");S.matcher=(()=>{let r="/",t="./",e=new RegExp(["(\\\\.)","(?:\\:(\\w+)(?:\\(((?:\\\\.|[^\\\\()])+)\\))?|\\(((?:\\\\.|[^\\\\()])+)\\))([+*?])?"].join("|"),"g");function i(u,b){let f=[],x=0,k=0,g="",E=b&&b.delimiter||r,N=b&&b.delimiters||t,$=!1,q;for(;(q=e.exec(u))!==null;){let D=q[0],A=q[1],L=q.index;if(g+=u.slice(k,L),k=L+D.length,A){g+=A[1],$=!0;continue}let C="",O=u[k],Ue=q[2],We=q[3],Ge=q[4],T=q[5];if(!$&&g.length){let et=g.length-1;N.indexOf(g[et])>-1&&(C=g[et],g=g.slice(0,et))}g&&(f.push(g),g="",$=!1);let Ze=C!==""&&O!==void 0&&O!==C,Je=T==="+"||T==="*",Ke=T==="?"||T==="*",Rt=C||E,Nt=We||Ge;f.push({name:Ue||x++,prefix:C,delimiter:Rt,optional:Ke,repeat:Je,partial:Ze,pattern:Nt?o(Nt):"[^"+s(Rt)+"]+?"})}return(g||k<u.length)&&f.push(g+u.substr(k)),f}a(i,"parse");function s(u){return u.replace(/([.+*?=^!:${}()[\]|/\\])/g,"\\$1")}a(s,"escapeString");function o(u){return u.replace(/([=!:$/()])/g,"\\$1")}a(o,"escapeGroup");function n(u){return u&&u.sensitive?"":"i"}a(n,"flags");function c(u,b){if(!b)return u;let f=u.source.match(/\((?!\?)/g);if(f)for(let x=0;x<f.length;x++)b.push({name:x,prefix:null,delimiter:null,optional:!1,repeat:!1,partial:!1,pattern:null});return u}a(c,"regexpToRegexp");function l(u,b,f){let x=[];for(let k=0;k<u.length;k++)x.push(m(u[k],b,f).source);return new RegExp("(?:"+x.join("|")+")",n(f))}a(l,"arrayToRegexp");function h(u,b,f){return d(i(u,f),b,f)}a(h,"stringToRegexp");function d(u,b,f){f=f||{};let x=f.strict,k=f.end!==!1,g=s(f.delimiter||r),E=f.delimiters||t,N=[].concat(f.endsWith||[]).map(s).concat("$").join("|"),$="",q=u.length===0;for(let D=0;D<u.length;D++){let A=u[D];if(typeof A=="string")$+=s(A),q=D===u.length-1&&E.indexOf(A[A.length-1])>-1;else{let L=s(A.prefix),C=A.repeat?"(?:"+A.pattern+")(?:"+L+"(?:"+A.pattern+"))*":A.pattern;b&&b.push(A),A.optional?A.partial?$+=L+"("+C+")?":$+="(?:"+L+"("+C+"))?":$+=L+"("+C+")"}}return k?(x||($+="(?:"+g+")?"),$+=N==="$"?"$":"(?="+N+")"):(x||($+="(?:"+g+"(?="+N+"))?"),q||($+="(?="+g+"|"+N+")")),new RegExp("^"+$,n(f))}a(d,"tokensToRegExp");function m(u,b,f){return u instanceof RegExp?c(u,b):Array.isArray(u)?l(u,b,f):h(u,b,f)}return a(m,"pathToRegexp"),m})();ye.exports={TonicRouter:S}});var ke=v((Cs,we)=>{var Ei=y(),M=class extends Ei{defaults(){return{disabled:!1,invalid:!1,iconArrow:M.svg.default(),width:"250px",radius:"2px"}}static stylesheet(){return`
      tonic-select .tonic--wrapper {
        position: relative;
      }

      tonic-select .tonic--wrapper:before {
        content: '';
        width: 14px;
        height: 14px;
        opacity: 0;
        z-index: 1;
      }

      tonic-select.tonic--loading {
        pointer-events: none;
        transition: background 0.3s ease;
      }

      tonic-select.tonic--loading select {
        color: transparent;
        background-color: var(--tonic-window, #fff);
        border-color: var(--tonic-border, #ccc);
      }

      tonic-select.tonic--loading .tonic--wrapper:before {
        margin-top: -8px;
        margin-left: -8px;
        display: block;
        position: absolute;
        bottom: 10px;
        left: 50%;
        opacity: 1;
        transform: translateX(-50%);
        border: 2px solid var(--tonic-medium, #999);
        border-radius: 50%;
        border-top-color: transparent;
        animation: spin 1s linear 0s infinite;
        transition: opacity 0.3s ease;
      }

      tonic-select select {
        color: var(--tonic-primary, #333);
        font: 14px var(--tonic-monospace, 'Andale Mono', monospace);
        background-color: var(--tonic-input-background, var(--tonic-background, #f66));
        background-repeat: no-repeat;
        background-position: center right;
        border: 1px solid var(--tonic-border, #ccc);
        -webkit-appearance: none;
        -moz-appearance: none;
        appearance: none;
        position: relative;
      }

      tonic-select select:focus {
        background-color: var(--tonic-input-background-focus, rgba(241, 241, 241, 0.75));
        outline: none;
      }

      tonic-select[edited] select[invalid],
      tonic-select[edited] select:invalid,
      tonic-select[edited] select[invalid]:focus,
      tonic-select[edited] select:invalid:focus {
        border-color: var(--tonic-error, #f66);
      }

      tonic-select[edited] select[invalid] ~ .tonic--invalid,
      tonic-select[edited] select:invalid ~ .tonic--invalid {
        transform: translateY(0);
        visibility: visible;
        opacity: 1;
        transition: opacity 0.2s ease, transform 0.2s ease, visibility 1s ease 0s;
      }

      tonic-select[label] .tonic--invalid {
        margin-bottom: -13px;
      }

      tonic-select .tonic--invalid {
        font-size: 14px;
        text-align: center;
        margin-bottom: 13px;
        position: absolute;
        bottom: 100%;
        left: 0;
        right: 0;
        transform: translateY(-10px);
        transition: opacity 0.2s ease, transform 0.2s ease, visibility 0s ease 1s;
        visibility: hidden;
        opacity: 0;
      }

      tonic-select .tonic--invalid span {
        color: white;
        padding: 10px;
        background-color: var(--tonic-error, #f66);
        border-radius: 2px;
        position: relative;
        display: inline-block;
        margin: 0 auto;
      }

      tonic-select .tonic--invalid span:after {
        content: '';
        width: 0;
        height: 0;
        display: block;
        position: absolute;
        bottom: -6px;
        left: 50%;
        transform: translateX(-50%);
        border-left: 6px solid transparent;
        border-right: 6px solid transparent;
        border-top: 6px solid var(--tonic-error, #f66);
      }

      tonic-select select:not([multiple]) {
        padding: 8px 30px 8px 8px;
      }

      tonic-select select[disabled] {
        background-color: transparent;
      }

      tonic-select label {
        color: var(--tonic-medium, #999);
        font: 12px/14px var(--tonic-subheader, 'Arial', sans-serif);
        font-weight: 500;
        text-transform: uppercase;
        letter-spacing: 1px;
        padding-bottom: 10px;
        display: block;
      }

      tonic-select[multiple] select {
        background-image: none !important;
      }

      tonic-select[multiple] select option {
        padding: 6px 8px;
      }

      @keyframes spin {
        from {
          transform: rotate(0deg);
        }
        to {
          transform: rotate(360deg);
        }
      }
    `}setValid(){let t=this.querySelector("select");!t||(t.setCustomValidity(""),t.removeAttribute("invalid"))}setInvalid(t){let e=this.querySelector("select");if(!e)return;this.setAttribute("edited",!0),t=t||this.props.errorMessage,e.setCustomValidity(t),window.requestAnimationFrame(()=>{e.setAttribute("invalid",t)});let i=this.querySelector(".tonic--invalid span");!i||(i.textContent=t)}get value(){let t=this.querySelector("select");return this.props.multiple==="true"?[...t.options].filter(i=>i.selected).map(i=>i.getAttribute("value")):t.value}selectOptions(t){[...this.querySelector("select").options].forEach(s=>{s.selected=t.findIndex(o=>o===s.value)>-1})}set value(t){let e=this.props.multiple==="true"&&Array.isArray(t),i=this.root.querySelector("select");e?this.selectOptions(t):t?i.value=t:(t=i.selectedIndex,i.selectedIndex=0)}get option(){let t=this.querySelector("select");return t.options[t.selectedIndex]}get selectedIndex(){return this.querySelector("select").selectedIndex}set selectedIndex(t){let e=this.querySelector("select");e.selectedIndex=t}loading(t){let e=t?"add":"remove";this.classList[e]("tonic--loading")}renderLabel(){return this.props.label?this.html`
      <label
        for="tonic--select_${this.props.id}"
      >${this.props.label}</label>
    `:""}styles(){let{height:t,width:e,padding:i,radius:s,iconArrow:o}=this.props;return{wrapper:{width:e},select:{width:e,height:t,borderRadius:s,padding:i,backgroundImage:`url('${o}')`}}}setupEvents(){this.querySelector("select").addEventListener("change",e=>{this.setAttribute("edited",!0)})}updated(){this.setupEvents()}connected(){let t=this.props.value;if(Array.isArray(t))this.selectOptions(t);else if(t){let e=this.querySelector("option[selected]");e&&e.removeAttribute("selected");let i=this.querySelector(`option[value="${t}"]`);i&&i.setAttribute("selected",!0)}}render(){let{width:t,height:e,disabled:i,required:s,multiple:o,theme:n,name:c,size:l,tabindex:h}=this.props;t&&(this.style.width=t),e&&(this.style.width=e),n&&this.classList.add(`tonic--theme--${n}`),c&&this.removeAttribute("name"),h&&this.removeAttribute("tabindex");let d=this.props.errorMessage||this.props.errormessage||"Invalid";return this.html`
      <div class="tonic--wrapper" styles="wrapper">
        ${this.renderLabel()}
        <select ... ${{styles:"select",disabled:i==="true",multiple:o==="true",name:c,tabindex:h,required:s,size:l,id:`tonic--select_${this.props.id}`}}>
          ${this.childNodes}
        </select>

        <div class="tonic--invalid">
          <span id="tonic--error-${this.props.id}">${d}</span>
        </div>
      </div>
    `}};a(M,"TonicSelect");M.svg={};M.svg.toURL=r=>`data:image/svg+xml;base64,${window.btoa(r)}`;M.svg.default=()=>M.svg.toURL(`
  <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
    <path fill="#D7DBDD" d="M61.4,45.8l-11,13.4c-0.2,0.3-0.5,0.3-0.7,0l-11-13.4c-0.3-0.3-0.1-0.8,0.4-0.8h22C61.4,45,61.7,45.5,61.4,45.8z"/>
  </svg>
`);we.exports={TonicSelect:M}});var Ae=v((Is,Ee)=>{var Ai=y(),wt=class extends Ai{static stylesheet(){return`
      tonic-sprite svg {
        visibility: hidden;
        height: 0;
      }
    `}render(){return this.html`
      <svg version="1.1" xmlns="http://www.w3.org/2000/svg">

        <symbol id="close" viewBox="0 0 100 100">
          <title>Close</title>
          <desc>Close Icon</desc>
          <path fill="currentColor" d="M70.7,22.6l-3.5-3.5c-0.1-0.1-0.3-0.1-0.4,0L50,45.9L23.2,19.1c-0.1-0.1-0.3-0.1-0.4,0l-3.5,3.5c-0.1,0.1-0.1,0.3,0,0.4
            l26.8,26.8L19.3,76.6c-0.1,0.1-0.1,0.3,0,0.4l3.5,3.5c0,0,0.1,0.1,0.2,0.1s0.1,0,0.2-0.1L50,53.6l25.9,25.9c0.1,0.1,0.3,0.1,0.4,0
            l3.5-3.5c0.1-0.1,0.1-0.3,0-0.4L53.9,49.8l26.8-26.8C80.8,22.8,80.8,22.7,80.7,22.6z"/>
        </symbol>

        <symbol id="danger" viewBox="0 0 100 100">
          <path fill="currentColor" d="M50.1,6.7C26.3,6.7,6.9,26.2,6.9,50s19.4,43.2,43.2,43.2c23.8,0,43.2-19.5,43.2-43.3C93.3,26.1,74,6.7,50.1,6.7z M53.9,76.4h-7.6V68h7.6V76.4z M53.9,60.5h-7.6V25.6h7.6V60.5z"/>
        </symbol>

        <symbol id="warning" viewBox="0 0 100 100">
          <path fill="currentColor" d="M98.6,86.6l-46-79.7c-1.2-2-4-2-5.2,0l-46,79.7c-1.2,2,0.3,4.5,2.6,4.5h92C98.3,91.1,99.8,88.6,98.6,86.6z M53.9,80.4h-7.6V72h7.6V80.4z M53.9,64.5h-7.6V29.6h7.6V64.5z"/>
        </symbol>

        <symbol id="success" viewBox="0 0 100 100">
          <path fill="currentColor" d="M50.1,6.7C26.3,6.7,6.9,26.2,6.9,50s19.4,43.2,43.2,43.2c23.8,0,43.2-19.5,43.2-43.3C93.3,26.1,74,6.7,50.1,6.7z M43.4,71.5L22,50.1l4.8-4.8L43.4,62l28.5-28.5l4.8,4.8L43.4,71.5z"/>
        </symbol>

        <symbol id="info" viewBox="0 0 100 100">
          <path fill="currentColor" d="M50.1,6.7C26.3,6.7,6.9,26.2,6.9,50s19.4,43.2,43.2,43.2c23.8,0,43.2-19.5,43.2-43.3C93.3,26.1,74,6.7,50.1,6.7z M54.1,75.5h-8.1v-7.8h8.1V75.5z M64.1,47.6c-0.8,1.1-2.4,2.7-4.8,4.5L57,54c-1.4,1.1-2.3,2.3-2.7,3.7c-0.3,0.8-0.4,2-0.4,3.6h-8c0.1-3.4,0.5-5.8,1-7.1c0.5-1.3,2-2.9,4.3-4.7l2.4-1.9c0.8-0.6,1.5-1.3,2-2.1c0.9-1.3,1.4-2.8,1.4-4.3c0-1.8-0.5-3.4-1.6-4.9c-1.1-1.5-3-2.3-5.8-2.3c-2.7,0-4.7,0.9-5.9,2.8c-1,1.6-1.6,3.3-1.7,5.1h-8.6c0.4-5.9,2.5-10.1,6.4-12.6l0,0c2.5-1.6,5.7-2.5,9.4-2.5c4.9,0,9,1.2,12.2,3.5c3.2,2.3,4.8,5.7,4.8,10.3C66.2,43.4,65.5,45.7,64.1,47.6z"/>
        </symbol>

        <symbol id="profileimage" viewBox="0 0 100 100">
          <rect fill="#F0F0F0" width="100" height="100"></rect>
          <circle fill="#D6D6D6" cx="49.3" cy="41.3" r="21.1"></circle>
          <path fill="#D6D6D6" d="M48.6,69.5c-18.1,0-33.1,13.2-36,30.5h72C81.8,82.7,66.7,69.5,48.6,69.5z"></path>
        </symbol>

        <symbol id="edit" viewBox="0 0 100 100">
          <path fill="currentColor" d="M79.8,32.8L67.5,20.5c-0.2-0.2-0.5-0.2-0.7,0L25.2,62.1c-0.1,0.1-0.1,0.1-0.1,0.2L20.8,79c0,0.2,0,0.4,0.1,0.5c0.1,0.1,0.2,0.1,0.4,0.1c0,0,0.1,0,0.1,0l16.6-4.4c0.1,0,0.2-0.1,0.2-0.1l41.6-41.6C79.9,33.3,79.9,33,79.8,32.8z M67.1,25.8l7.3,7.3L36.9,70.7l-7.3-7.3L67.1,25.8z M33,72.4l-6.8,1.8l1.8-6.9L33,72.4z"/>
        </symbol>

        <symbol id="checked" viewBox="0 0 100 100">
          <path fill="currentColor" d="M79.7,1H21.3C10.4,1,1.5,9.9,1.5,20.8v58.4C1.5,90.1,10.4,99,21.3,99h58.4c10.9,0,19.8-8.9,19.8-19.8V20.8C99.5,9.9,90.6,1,79.7,1z M93.3,79.3c0,7.5-6.1,13.6-13.6,13.6H21.3c-7.5,0-13.6-6.1-13.6-13.6V20.9c0-7.5,6.1-13.6,13.6-13.6V7.2h58.4c7.5,0,13.6,6.1,13.6,13.6V79.3z"/>
          <polygon points="44,61.7 23.4,41.1 17.5,47 44,73.5 85.1,32.4 79.2,26.5 "/>
        </symbol>

        <symbol id="download" viewBox="0 0 100 100">
          <path fill="currentColor" d="M49.5,64.2c0.1,0.1,0.2,0.1,0.4,0.1s0.3-0.1,0.4-0.1l12.4-12.4c0.1-0.1,0.1-0.2,0.1-0.4s-0.1-0.3-0.1-0.4l-2.1-2.1
            c-0.2-0.2-0.5-0.2-0.7,0l-7.9,7.9V27c0-0.3-0.2-0.5-0.5-0.5h-3c-0.3,0-0.5,0.2-0.5,0.5v29.8L40,48.9c-0.1-0.1-0.2-0.1-0.4-0.1
            s-0.3,0.1-0.4,0.1l-2.1,2.1c-0.2,0.2-0.2,0.5,0,0.7L49.5,64.2z"/>
          <path fill="currentColor" d="M68.9,69h-38c-0.3,0-0.5,0.2-0.5,0.5v3c0,0.3,0.2,0.5,0.5,0.5h38c0.3,0,0.5-0.2,0.5-0.5v-3C69.4,69.2,69.2,69,68.9,69z"/>
        </symbol>

        <symbol id="unchecked" viewBox="0 0 100 100">
          <path fill="currentColor" d="M79.7,99H21.3C10.4,99,1.5,90.1,1.5,79.2V20.8C1.5,9.9,10.4,1,21.3,1h58.4c10.9,0,19.8,8.9,19.8,19.8v58.4C99.5,90.1,90.6,99,79.7,99z M21.3,7.3c-7.5,0-13.6,6.1-13.6,13.6v58.4c0,7.5,6.1,13.6,13.6,13.6h58.4c7.5,0,13.6-6.1,13.6-13.6V20.8c0-7.5-6.1-13.6-13.6-13.6H21.3V7.3z"/>
        </symbol>

      </svg>
    `}};a(wt,"TonicSprite");Ee.exports={TonicSprite:wt}});var Se=v((_s,$e)=>{var Z=y(),J=class extends Z{render(){return this.props.width&&(this.style.width=this.props.width),this.html`
      ${this.elements}
    `}};a(J,"TonicSplitLeft");var K=class extends Z{render(){return this.props.height&&(this.style.height=this.props.height),this.html`
      ${this.elements}
    `}};a(K,"TonicSplitTop");var kt=class extends J{};a(kt,"TonicSplitRight");var Et=class extends K{};a(Et,"TonicSplitBottom");var At=class extends Z{constructor(){super();this.left=null,this.right=null,this.top=null,this.bottom=null,this.handleId=`tonic--handle-${Math.random().toString().slice(2)}`,this.state.meta={}}static stylesheet(){return`
      tonic-split {
        position: absolute;
        top: 0;
        bottom: 0;
        left: 0;
        right: 0;
      }

      tonic-split > tonic-split-top,
      tonic-split > tonic-split-bottom,
      tonic-split > tonic-split-left,
      tonic-split > tonic-split-right {
        position: absolute;
        overflow: hidden;
      }

      tonic-split > tonic-split-left,
      tonic-split > tonic-split-right {
        top: 0;
        bottom: 0;
      }

      tonic-split > tonic-split-left {
        left: 0;
        right: unset;
        width: 60%;
      }

      tonic-split > tonic-split-right {
        right: 0;
        left: unset;
        width: 40%;
      }

      tonic-split > tonic-split-top,
      tonic-split > tonic-split-bottom {
        left: 0;
        height: 50%;
        right: 0;
      }

      tonic-split > tonic-split-bottom {
        bottom: 0;
        top: unset;
      }

      tonic-split > tonic-split-top {
        top: 0;
        bottom: unset;
        z-index: 4;
      }

      tonic-split > tonic-split-right {
        right: 0;
        left: unset;
        width: 40%;
      }

      tonic-split .tonic--split-handle {
        position: absolute;
        z-index: 1;
        user-select: none;
        -webkit-user-select: none;
        background-color: transparent;
        transition: background .1s ease;
      }

      #split-query > div {
        z-index: 101;
        margin-top: -5px;
      }

      tonic-split .tonic--split-vertical {
        top: 0;
        bottom: 0;
        left: 60%;
        width: 5px;
        border-left: 1px solid var(--tonic-border);
        cursor: ew-resize;
      }

      tonic-split .tonic--split-horizontal {
        left: 0;
        right: 0;
        height: 5px;
        top: 50%;
        border-bottom: 1px solid var(--tonic-border);
        cursor: ns-resize;
      }

      tonic-split[dragging] > .tonic--split-handle,
      tonic-split .tonic--split-handle:hover {
        background: var(--tonic-accent);
      }

      tonic-split[dragging],
      tonic-split[dragging] * {
        cursor: -webkit-grabbing;
        cursor: -moz-grabbing;
        cursor: grabbing;
      }

      tonic-split[dragging] * {
        pointer-events: none;
        user-select: none;
        -webkit-user-select: none;
      }
    `}start(){this.dragging=!0,this.setAttribute("dragging",!0)}cancel(){this.dragging=!1,this.removeAttribute("dragging")}willConnect(){this.updated()}hide(t){this[t].hidden||this.toggle(t,!1)}show(t){!this[t].hidden||this.toggle(t,!0)}toggle(t,e){let{meta:i}=this.state;typeof e=="boolean"&&e===!1&&delete i[t];let s=i[t],o="",n="";if(this.props.type==="vertical"?(o=t==="left"?"right":"left",n="width"):(o=t==="top"?"bottom":"top",n="height"),!s&&!e){i[t]={[t]:this[t].style[n],[o]:this[o].style[n],handle:this.handle.style.display},this[t].style[n]=0,this[o].style[n]="100%",this.handle.style.display="none";return}s&&(this[t].style[n]=s[t],this[o].style[n]=s[o],this.handle.style.display=s.handle,delete i[t])}connected(){this.handle=this.querySelector(`#${this.handleId}`),this.props.type==="vertical"?(this.left=this.elements[0],this.right=this.elements[1],this.handle.style.left=this.left.getAttribute("width")):(this.top=this.elements[0],this.bottom=this.elements[1],this.handle.style.top=this.top.getAttribute("height"))}afterResize(){clearTimeout(this.timeout),this.timeout=setTimeout(()=>{this.dispatchEvent(new window.CustomEvent("resize",{bubbles:!0}))},64)}updated(){this.cancel()}disconnected(){this.handle=null,this.left=null,this.right=null,this.top=null,this.bottom=null}mousemove(t){if(!this.dragging)return;let{x:e,y:i}=this.getBoundingClientRect(),s=this.offsetWidth,o=this.offsetHeight;this.lastX=t.clientX+1,this.lastY=t.clientY-1;let n=parseInt(this.props.max,10)||25,c=parseInt(this.props.min,10)||25;if(this.props.type==="vertical"){this.left=this.elements[0],this.right=this.elements[1];let d=t.clientX-e;d>=s-n&&(d=s-n),d<=c&&(d=c);let m=d/s*100;this.left.style.width=m+"%",this.handle.style.left=m+"%",this.right.style.width=100-m+"%",this.afterResize();return}this.top=this.elements[0],this.bottom=this.elements[1];let l=t.clientY-i;l>=o-n&&(l=o-n),l<=c&&(l=c);let h=l/o*100;h<=100-5&&(this.top.style.height=h+"%",this.handle.style.top=h+"%",this.bottom.style.height=100-h+"%",this.afterResize())}mouseenter(t){t.buttons===0&&this.cancel()}mouseleave(t){t.buttons===0&&this.cancel()}mousedown(t){let e=Z.match(t.target,".tonic--split-handle");e&&e.parentElement===this&&(this.handle=e,this.start())}mouseup(t){this.cancel()}render(){let t=["tonic--split-handle",`tonic--split-${this.props.type}`].join(" ");return this.html`
      ${this.elements[0]}

      <div class="${t}" id="${this.handleId}">
      </div>

      ${this.elements[1]}
    `}};a(At,"TonicSplit");$e.exports={TonicSplit:At,TonicSplitLeft:J,TonicSplitRight:kt,TonicSplitTop:K,TonicSplitBottom:Et}});var Ce=v((zs,Le)=>{var H=y(),qe=window.CustomEvent,$t=class extends H{constructor(t){super(t);this._setVisibilitySynchronously=!1,this.panels={}}static stylesheet(){return`
      tonic-tabs .tonic--tab {
        -webkit-appearance: none;
        -webkit-user-select: none;
        user-select: none;
      }

      tonic-tab a:focus {
        outline: none;
        background-color: var(--tonic-input-background);
      }
    `}get value(){let t=this.querySelector('[aria-selected="true"]');if(t)return t.parentNode.id}set selected(t){let e=document.getElementById(t);e&&this.setVisibility(e.id,e.getAttribute("for"))}get selected(){return this.value}willConnect(){this.panels=this.panels||{}}async setVisibility(t,e){let i=this.props.renderAll==="true",s=this.props.detatchOnHide,o=this.querySelectorAll("tonic-tab");for(let n of o){let c=n.getAttribute("for"),l=n.querySelector("a");if(!c)throw new Error(`No "for" attribute found for tab id "${n.id}".`);let h=this.panels[c],d=document.getElementById(c);if(!d&&h&&(d=h.node),!d||!l){if(this._setVisibilitySynchronously)return setTimeout(()=>this.setVisibility(t,e));throw new Error(`No tonic-panel found that matches the id (${c})`)}n.id===t||c===e?(d.removeAttribute("hidden"),n.id===t?l.setAttribute("aria-selected","true"):l.setAttribute("aria-selected","false"),d.visible||(d.visible=!0,d.parentElement&&d.reRender&&s&&await d.reRender()),!d.parentElement&&s&&(h.parent.appendChild(d),d.preventRenderOnReconnect&&d.reRender&&d.children.length===0&&await d.reRender()),this.state.selected=t,this._setVisibilitySynchronously||this.dispatchEvent(new qe("tabvisible",{detail:{id:t},bubbles:!0}))):(!d.visible&&i&&s&&(d.visible=!0,d.parentElement&&d.reRender&&await d.reRender()),d.setAttribute("hidden",""),s&&d.parentElement&&!i&&(this.panels[d.id]={parent:d.parentElement,node:d},d.remove()),l&&l.setAttribute("aria-selected","false"),this._setVisibilitySynchronously||this.dispatchEvent(new qe("tabhidden",{detail:{id:t},bubbles:!0})))}}click(t){let e=H.match(t.target,".tonic--tab");!e||(t.preventDefault(),this.setVisibility(e.parentNode.id,e.getAttribute("for")))}keydown(t){let e=[...this.querySelectorAll(".tonic--tab")];switch(t.code){case"ArrowLeft":case"ArrowRight":{let i=e.indexOf(t.target),s=t.code==="ArrowLeft"?-1:1,o=e.length,n=(i+o+s)%o;e[n].focus(),t.preventDefault();break}case"Space":{let i=H.match(t.target,".tonic--tab:focus");if(!i)return;t.preventDefault();let s=i.parentNode.getAttribute("id");this.setVisibility(s,i.getAttribute("for"));break}}}connected(){let t=this.state.selected||this.props.selected;if(!t)throw new Error("missing selected property.");this._setVisibilitySynchronously=!0,this.setVisibility(t),this._setVisibilitySynchronously=!1}render(){return this.setAttribute("role","tablist"),this.html`${this.childNodes}`}};a($t,"TonicTabs");var St=class extends H{static stylesheet(){return`
      tonic-tab-panel {
        display: block;
      }

      tonic-tab-panel[hidden] {
        display: none;
      }
    `}constructor(t){super(t);this.visible=this.visible||!1,this.__originalChildren=this.nodes,this.visible||this.setAttribute("hidden",""),this.setAttribute("role","tabpanel")}connected(){let t=document.querySelector(`.tonic--tab[for="${this.props.id}"]`);if(t){let e=t.getAttribute("id");this.setAttribute("aria-labelledby",e)}}disconnected(){this.props.detatch&&(this.preventRenderOnReconnect=!0)}render(){return!this.visible&&this.props.detatch?"":this.props.detatch?this.html`${this.__originalChildren}`:this.html`${this.childNodes}`}};a(St,"TonicTabPanel");var qt=class extends H{render(){let t=this.props.for;return this.html`
      <a
        id="${this.id}-anchor"
        for="${this.props.for}"
        class="tonic--tab"
        href="#"
        role="tab"
        aria-controls="${t}"
        aria-selected="false"
      >
        ${this.childNodes}
      </a>
    `}};a(qt,"TonicTab");Le.exports={TonicTabs:$t,TonicTab:qt,TonicTabPanel:St}});var _e=v((Ms,Ie)=>{var $i=y(),Lt=class extends $i{mutate(t){let{width:e,height:i}=t.target.style;this.state={...this.state,width:e,height:i}}defaults(){return{placeholder:"",spellcheck:!0,disabled:!1,required:!1,readonly:!1,autofocus:!1,width:"100%",radius:"2px"}}static stylesheet(){return`
      tonic-textarea {
        display: block;
        position: relative;
      }

      tonic-textarea textarea {
        color: var(--tonic-primary, #333);
        width: 100%;
        font: 14px var(--tonic-monospace, 'Andale Mono', monospace);
        padding: 10px;
        background-color: var(--tonic-input-background, var(--tonic-background, transparent));
        border: 1px solid var(--tonic-border, #ccc);
        transition: border 0.2s ease-in-out;
        -webkit-appearance: none;
        -moz-appearance: none;
        appearance: none;
      }

      tonic-textarea textarea:invalid {
        border-color: var(--tonic-danger, #f66);
      }

      tonic-textarea textarea:focus {
        outline: none;
        background-color: var(--tonic-input-background-focus, rgba(241, 241, 241, 0.75));
      }

      tonic-textarea textarea[disabled] {
        background-color: transparent;
      }

      tonic-textarea label {
        color: var(--tonic-medium, #999);
        font-weight: 500;
        font: 12px/14px var(--tonic-subheader,  'Arial', sans-serif);
        text-transform: uppercase;
        letter-spacing: 1px;
        padding-bottom: 10px;
        display: block;
      }

      tonic-textarea[edited] textarea[invalid],
      tonic-textarea[edited] textarea:invalid,
      tonic-textarea[edited] textarea[invalid]:focus,
      tonic-textarea[edited] textarea:invalid:focus {
        border-color: var(--tonic-error, #f66);
      }

      tonic-textarea[edited] textarea[invalid] ~ .tonic--invalid,
      tonic-textarea[edited] textarea:invalid ~ .tonic--invalid {
        transform: translateY(0);
        visibility: visible;
        opacity: 1;
        transition: opacity 0.2s ease, transform 0.2s ease, visibility 1s ease 0s;
      }

      tonic-textarea textarea[disabled] {
        background-color: var(--tonic-background, #fff);
      }

      tonic-textarea[label] .tonic--invalid {
        margin-bottom: 13px;
      }

      tonic-textarea .tonic--invalid {
        font-size: 14px;
        text-align: center;
        margin-bottom: 13px;
        position: absolute;
        bottom: 100%;
        left: 0;
        right: 0;
        transform: translateY(-10px);
        transition: opacity 0.2s ease, transform 0.2s ease, visibility 0s ease 1s;
        visibility: hidden;
        opacity: 0;
      }

      tonic-textarea .tonic--invalid span {
        color: white;
        padding: 2px 6px;
        background-color: var(--tonic-error, #f66);
        border-radius: 2px;
        position: relative;
        display: inline-block;
        margin: 0 auto;
      }

      tonic-textarea .tonic--invalid span:after {
        content: '';
        width: 0;
        height: 0;
        display: block;
        position: absolute;
        bottom: -6px;
        left: 50%;
        transform: translateX(-50%);
        border-left: 6px solid transparent;
        border-right: 6px solid transparent;
        border-top: 6px solid var(--tonic-error, #f66);
      }

    `}styles(){let{width:t=this.state.width,height:e=this.state.height,radius:i,resize:s}=this.props;return{textarea:{width:t,height:e,borderRadius:i,resize:s}}}get value(){return this.querySelector("textarea").value}set value(t){this.querySelector("textarea").value=t}setValid(){let t=this.querySelector("textarea");!t||(t.setCustomValidity(""),t.removeAttribute("invalid"))}setInvalid(t){let e=this.querySelector("textarea");if(!e)return;this.setAttribute("edited",!0),this.state.edited=!0,e.setCustomValidity(t),window.requestAnimationFrame(()=>{e.setAttribute("invalid",t)});let i=this.querySelector(".tonic--invalid span");!i||(i.textContent=t)}renderLabel(){return typeof this.props.label=="undefined"?"":this.html`
      <label
        for="tonic--textarea_${this.props.id}"
      >${this.props.label}</label>
    `}willConnect(){let{value:t,persistSize:e}=this.props;if(this.props.value=typeof t=="undefined"?this.textContent:t,e==="true"){let i=a(s=>this.mutate(s.pop()),"cb");this.observer=new window.MutationObserver(i).observe(this,{attributes:!0,subtree:!0,attributeFilter:["style"]})}}disconnected(){this.observer&&this.observer.disconnect()}keyup(t){if(!this.props.pattern)return;this.regex||(this.regex=new RegExp(this.props.pattern)),t.target.value.trim().match(this.regex)?this.setValid():this.setInvalid("Invalid")}render(){let{ariaLabelledby:t,autofocus:e,cols:i,height:s,disabled:o,label:n,maxlength:c,minlength:l,name:h,placeholder:d,readonly:m,required:u,rows:b,spellcheck:f,tabindex:x,theme:k,width:g}=this.props;t&&this.removeAttribute("ariaLabelled"),g&&(this.style.width=g),s&&(this.style.height=s),x&&this.removeAttribute("tabindex"),k&&this.classList.add(`tonic--theme--${k}`),h&&this.removeAttribute("name");let E=this.props.errorMessage||"Invalid";return this.props.value==="undefined"&&(this.props.value=""),this.state.edited&&this.setAttribute("edited",!0),this.html`
      <div class="tonic--wrapper">
        ${this.renderLabel()}
        <textarea ... ${{styles:"textarea",id:`tonic--textarea_${this.props.id}`,"aria-label":n,"aria-labelledby":t,cols:i,disabled:o==="true",maxlength:c,minlength:l,name:h,placeholder:d,rows:b,spellcheck:f,tabindex:x,autofocus:e,required:u,readonly:m}}>${this.props.value}</textarea>
        <div class="tonic--invalid">
          <span id="tonic--error-${this.props.id}">${E}</span>
        </div>
      </div>
    `}};a(Lt,"TonicTextarea");Ie.exports={TonicTextarea:Lt}});var Me=v((Rs,ze)=>{var Si=y(),I=class extends Si{connected(){let t=this.props.for,e=document.getElementById(t),i=a(o=>{clearTimeout(I.timers[t]),I.timers[t]=setTimeout(()=>this.hide(),256)},"leave"),s=a(o=>{clearTimeout(I.timers[t]),this.show(e)},"enter");if(!e){console.warn("Broken tooltip for: "+t);return}e.addEventListener("mouseleave",i),e.addEventListener("mousemove",s),e.addEventListener("mouseenter",s),this.addEventListener("mouseenter",o=>{clearTimeout(I.timers[t])}),this.addEventListener("mouseleave",i)}defaults(t){return{width:"auto",height:"auto"}}static stylesheet(){return`
      tonic-tooltip .tonic--tooltip {
        color: var(--tonic-primary, #333);
        position: fixed;
        background: var(--tonic-window, #fff);
        visibility: hidden;
        z-index: -1;
        opacity: 0;
        border: 1px solid var(--tonic-border, #ccc);
        border-radius: 2px;
        transition: visibility 0.2s ease, opacity 0.2s ease, z-index 0.2s ease, box-shadow 0.2s ease;
        willchange: visibility, opacity, box-shadow, z-index;
      }

      tonic-tooltip .tonic--tooltip.tonic--show {
        visibility: visible;
        opacity: 1;
        z-index: 1;
        box-shadow: 0px 30px 90px -20px rgba(0, 0, 0, 0.3);
      }

      tonic-tooltip .tonic--tooltip .tonic--tooltip-arrow {
        width: 12px;
        height: 12px;
        position: absolute;
        z-index: -1;
        background-color: var(--tonic-window, #fff);
        -webkit-transform: rotate(45deg);
        -ms-transform: rotate(45deg);
        transform: rotate(45deg);
        left: 50%;
      }

      tonic-tooltip .tonic--tooltip .tonic--tooltip-arrow {
        border: 1px solid transparent;
        border-radius: 2px;
        pointer-events: none;
      }

      tonic-tooltip .tonic--top .tonic--tooltip-arrow {
        margin-bottom: -6px;
        bottom: 100%;
        border-top-color: var(--tonic-border, #ccc);
        border-left-color: var(--tonic-border, #ccc);
      }

      tonic-tooltip .tonic--bottom .tonic--tooltip-arrow {
        margin-top: -6px;
        position: absolute;
        top: 100%;
        border-bottom-color: var(--tonic-border, #ccc);
        border-right-color: var(--tonic-border, #ccc);
      }
    `}show(t){clearTimeout(I.timers[t.id]),I.timers[t.id]=setTimeout(()=>{let e=this.querySelector(".tonic--tooltip"),i=this.querySelector(".tonic--tooltip-arrow"),{top:s,left:o}=t.getBoundingClientRect();o+=t.offsetWidth/2,o-=e.offsetWidth/2;let n=t.offsetHeight+i.offsetHeight/2;s<window.innerHeight/2?(e.classList.remove("tonic--bottom"),e.classList.add("tonic--top"),s+=n):(e.classList.remove("tonic--top"),e.classList.add("tonic--bottom"),s-=n+e.offsetHeight),e.style.top=`${s}px`,e.style.left=`${o}px`,window.requestAnimationFrame(()=>{e.classList.add("tonic--show")}),window.addEventListener("mousewheel",c=>{this.hide()},{once:!0})},64)}hide(t){this.firstElementChild.classList.remove("tonic--show")}styles(){let{width:t,height:e}=this.props;return{tooltip:{width:t,height:e}}}render(){return this.props.theme&&this.classList.add(`tonic--theme--${this.props.theme}`),this.html`
      <div class="tonic--tooltip" styles="tooltip">
        ${this.nodes}
        <span class="tonic--tooltip-arrow"></span>
      </div>
    `}};a(I,"TonicTooltip");I.timers={};ze.exports={TonicTooltip:I}});var De=v((Ns,Ne)=>{var Re=y(),Ct=class extends Re{defaults(){return{display:"true"}}static stylesheet(){return`
      tonic-toaster-inline > div > .tonic--close svg,
      tonic-toaster-inline > div > .tonic--icon svg {
        width: inherit;
        height: inherit;
      }

      tonic-toaster-inline .tonic--notification {
        max-height: 0;
        background-color: var(--tonic-window, #fff);
        border: 1px solid var(--tonic-border, #ccc);
        border-radius: 3px;
        transform: translateY(-50%);
        transition: all 0.1s ease-in-out;
        opacity: 0;
        z-index: -1;
        position: absolute;
      }

      tonic-toaster-inline .tonic--notification.tonic--show {
        max-height: 100%;
        margin: 10px 0;
        transform: translateY(0);
        transition: all 0.1s ease-in-out;
        opacity: 1;
        z-index: 1;
        position: relative;
      }

      tonic-toaster-inline[animate="false"] .tonic--notification,
      tonic-toaster-inline[animate="false"] .tonic--notification.tonic--show {
        transition: none;
      }

      tonic-toaster-inline .tonic--notification.tonic--close {
        padding-right: 50px;
      }

      tonic-toaster-inline .tonic--notification.tonic--alert {
        padding-left: 35px;
      }

      tonic-toaster-inline .tonic--main {
        padding: 17px 18px 15px 18px;
      }

      tonic-toaster-inline.tonic--dismiss .tonic--main {
        margin-right: 40px;
      }

      tonic-toaster-inline .tonic--title {
        color: var(--tonic-primary, #333);
        font: 14px/18px var(--tonic-subheader, 'Arial', sans-serif);
      }

      tonic-toaster-inline .tonic--message {
        font: 14px/18px var(--tonic-subheader, 'Arial', sans-serif);
        color: var(--tonic-medium, #999);
      }

      tonic-toaster-inline .tonic--notification .tonic--icon {
        width: 16px;
        height: 16px;
        position: absolute;
        left: 20px;
        top: 50%;
        -webkit-transform: translateY(-50%);
        -ms-transform: translateY(-50%);
        transform: translateY(-50%);
      }

      tonic-toaster-inline .tonic--close {
        width: 20px;
        height: 20px;
        position: absolute;
        right: 20px;
        top: 50%;
        -webkit-transform: translateY(-50%);
        -ms-transform: translateY(-50%);
        transform: translateY(-50%);
        cursor: pointer;
      }
    `}show(){this.querySelector(".tonic--notification").classList.add("tonic--show")}hide(){this.querySelector(".tonic--notification").classList.remove("tonic--show")}click(t){!Re.match(t.target,".tonic--close")||this.hide()}connected(){this.updated()}updated(){let{display:t,duration:e}=this.props;t==="true"&&window.requestAnimationFrame(()=>this.show()),e&&setTimeout(()=>this.hide(),e)}renderClose(){return this.props.dismiss!=="true"?"":(this.classList.add("tonic--dismiss"),this.html`
      <div class="tonic--close">
        <svg>
          <use
            href="#close"
            xlink:href="#close"
            color="var(--tonic-primary, #333)"
            fill="var(--tonic-primary, #333)">
          </use>
        </svg>
      </div>
    `)}renderIcon(){let t=this.props.type;return t?this.html`
      <div class="tonic--icon">
        <svg>
          <use
            href="#${t}"
            xlink:href="#${t}"
            color="var(--tonic-${t}, #f66)"
            fill="var(--tonic-${t}, #f66)">
          </use>
        </svg>
      </div>
    `:""}styles(){return{type:{"border-color":`var(--tonic-${this.props.type}, #f66)`}}}render(){let{title:t,type:e,message:i,theme:s}=this.props;s&&this.setAttribute("theme",s);let o=["tonic--notification"];return e&&o.push("tonic--alert",`tonic--${e}`),this.html`
      <div ... ${{class:o.join(" "),styles:e?"type":""}}>
        ${this.renderIcon()}
        ${this.renderClose()}
        <div class="tonic--main">
          <div class="tonic--title">
            ${t}
          </div>
          <div class="tonic--message">
            ${i||this.childNodes}
          </div>
        </div>
      </div>
    `}};a(Ct,"TonicToasterInline");Ne.exports={TonicToasterInline:Ct}});var Oe=v((Ds,je)=>{var Ve=y(),It=class extends Ve{defaults(){return{position:"center"}}static stylesheet(){return`
      tonic-toaster * {
        box-sizing: border-box;
      }

      tonic-toaster svg {
        width: 100%;
        height: 100%;
        position: absolute;
      }

      tonic-toaster .tonic--wrapper {
        -webkit-user-select: none;
        user-select: none;
        position: fixed;
        top: 10px;
        display: flex;
        flex-wrap: wrap;
        flex-direction: column;
        visibility: hidden;
        z-index: 102;
      }

      @media (max-width: 850px) tonic-toaster .tonic--wrapper {
        width: 90%;
      }

      tonic-toaster .tonic--wrapper.tonic--show {
        visibility: visible;
      }

      tonic-toaster .tonic--wrapper.tonic--center {
        left: 50%;
        align-items: center;
        -webkit-transform: translateX(-50%);
        -ms-transform: translateX(-50%);
        transform: translateX(-50%);
      }

      tonic-toaster .tonic--wrapper.tonic--left {
        align-items: flex-start;
        left: 10px;
      }

      tonic-toaster .tonic--wrapper.tonic--right {
        align-items: flex-end;
        right: 10px;
      }

      tonic-toaster .tonic--notification {
        width: auto;
        max-width: 600px;
        margin-top: 10px;
        position: relative;
        background-color: var(--tonic-window);
        box-shadow: 0px 10px 40px -20px rgba(0,0,0,0.4), 0 0 1px #a2a9b1;
        border-radius: 3px;
        -webkit-transform: translateY(-100px);
        -ms-transform: translateY(-100px);
        transform: translateY(-100px);
        transition: opacity 0.2s ease, transform 0s ease 1s;
        z-index: 1;
        opacity: 0;
      }

      tonic-toaster .tonic--notification.tonic--show {
        opacity: 1;
        -webkit-transform: translateY(0);
        -ms-transform: translateY(0);
        transform: translateY(0);
        transition: transform 0.3s cubic-bezier(0.18, 0.89, 0.32, 1.28);
      }

      tonic-toaster .tonic--notification.tonic--close {
        padding-right: 50px;
      }

      tonic-toaster .tonic--notification.tonic--alert {
        padding-left: 35px;
      }

      tonic-toaster .tonic--main {
        padding: 17px 15px 15px 15px;
      }

      tonic-toaster .tonic--title {
        color: var(--tonic-primary);
        font: 14px/18px var(--tonic-subheader);
      }

      tonic-toaster .tonic--message {
        color: var(--tonic-medium);
        font: 14px/18px var(--tonic-body);
      }

      tonic-toaster .tonic--notification > .tonic--icon {
        width: 16px;
        height: 16px;
        position: absolute;
        left: 20px;
        top: 50%;
        -webkit-transform: translateY(-50%);
        -ms-transform: translateY(-50%);
        transform: translateY(-50%);
      }

      tonic-toaster .tonic--notification > .tonic--close {
        width: 20px;
        height: 20px;
        position: absolute;
        right: 20px;
        top: 50%;
        -webkit-transform: translateY(-50%);
        -ms-transform: translateY(-50%);
        transform: translateY(-50%);
        cursor: pointer;
      }
    `}_getZIndex(){return Array.from(document.querySelectorAll("body *")).map(t=>parseFloat(window.getComputedStyle(t).zIndex)).reduce((t,e=Number.MIN_SAFE_INTEGER)=>isNaN(t)||t<e?e:t)}create(t){let e=JSON.stringify(t);if(this.lastToaster===e)return;this.lastToaster=e,t=t||{};let{message:i,title:s,duration:o,type:n}=t,c=t.dismiss,l=document.createElement("div");l.className="tonic--notification";let h=this.querySelector(".tonic--wrapper");h.style.zIndex=this._getZIndex()+100;let d=document.createElement("div");d.className="tonic--main",n&&(l.dataset.type=n,l.classList.add("tonic--alert"));let m=document.createElement("div");m.className="tonic--title",m.textContent=s||this.props.title;let u=document.createElement("div");u.className="tonic--message";let b=i||this.props.message;if(i.isTonicTemplate?u.innerHTML=b:u.textContent=b,typeof c=="string"&&(c=c==="true"),c!==!1){let f=document.createElement("div");f.className="tonic--close",l.appendChild(f),l.classList.add("tonic--close");let x="http://www.w3.org/2000/svg",k="http://www.w3.org/1999/xlink",g=document.createElementNS(x,"svg"),E=document.createElementNS(x,"use");f.appendChild(g),g.appendChild(E),E.setAttributeNS(k,"href","#close"),E.setAttributeNS(k,"xlink:href","#close"),E.setAttribute("color","var(--tonic-primary)"),E.setAttribute("fill","var(--tonic-primary)")}if(n){let f=document.createElement("div");f.className="tonic--icon",l.appendChild(f);let x="http://www.w3.org/2000/svg",k="http://www.w3.org/1999/xlink",g=document.createElementNS(x,"svg"),E=document.createElementNS(x,"use");f.appendChild(g),g.appendChild(E),E.setAttributeNS(k,"href",`#${n}`),E.setAttributeNS(k,"xlink:href",`#${n}`),E.setAttribute("color",`var(--tonic-${n})`),E.setAttribute("fill",`var(--tonic-${n})`)}l.appendChild(d),d.appendChild(m),d.appendChild(u),this.querySelector(".tonic--wrapper").appendChild(l),this.show(),setTimeout(()=>{!l||l.classList.add("tonic--show")},64),o&&setTimeout(()=>{!l||this.destroy(l)},o)}destroy(t){t.classList.remove("tonic--show"),t.addEventListener("transitionend",e=>{t&&t.parentNode&&t.parentNode.removeChild(t)})}show(){this.querySelector(".tonic--wrapper").classList.add("tonic--show")}hide(){this.querySelector(".tonic--wrapper").classList.remove("tonic--show")}click(t){let e=Ve.match(t.target,".tonic--close");if(!e)return;let i=e.closest(".tonic--notification");i&&this.destroy(i)}render(){let{theme:t,position:e}=this.props;t&&this.classList.add(`tonic--theme--${t}`);let i=["tonic--wrapper"];return e&&i.push(`tonic--${e}`),this.html`
      <div class="${i.join(" ")}">
      </div>
    `}};a(It,"TonicToaster");je.exports={TonicToaster:It}});var Ye=v((Vs,Fe)=>{var qi=y(),_t=class extends qi{constructor(){super();this._modified=!1}get value(){let t=this.state,e=this.props,i=typeof e.checked!="undefined"?e.checked:e.value,s=typeof t.checked!="undefined"?t.checked:t.value,o;return this._modified?o=typeof s!="undefined"?s:i:o=typeof i!="undefined"?i:s,o===!0||o==="true"}_setValue(t){this._modified=!0;let e=t===!0||t==="true";this.state.checked=e}set value(t){this._setValue(t),this.reRender()}static stylesheet(){return`
      tonic-toggle .tonic--toggle--wrapper {
        height: 30px;
        position: relative;
      }

      tonic-toggle .tonic--toggle--wrapper > label {
        color: var(--tonic-medium, #999);
        font-weight: 500;
        font: 12px/14px var(--tonic-subheader, 'Arial', sans-serif);
        text-transform: uppercase;
        letter-spacing: 1px;
        margin-left: 58px;
        padding-top: 6px;
        display: block;
        user-select: none;
        -webkit-user-select: none;
      }

      tonic-toggle .tonic--switch {
        position: absolute;
        left: 0;
        top: 0;
      }

      tonic-toggle .tonic--switch label:before {
        font: bold 12px var(--tonic-subheader, 'Arial', sans-serif);
        text-transform: uppercase;
      }

      tonic-toggle .tonic--toggle {
        position: absolute;
        opacity: 0;
        outline: none;
        user-select: none;
        -webkit-user-select: none;
        z-index: 1;
      }

      tonic-toggle .tonic--toggle + label {
        width: 42px;
        height: 24px;
        padding: 2px;
        display: block;
        position: relative;
        background-color: var(--tonic-border, #ccc);
        border-radius: 60px;
        transition: background 0.2s ease-in-out;
        cursor: default;
      }

      tonic-toggle .tonic--toggle:focus + label {
        outline: -webkit-focus-ring-color auto 5px;
        outline-offset: 4px;
      }

      tonic-toggle .tonic--toggle + label:before {
        content: '';
        line-height: 29px;
        text-indent: 29px;
        position: absolute;
        top: 1px;
        left: 1px;
        right: 1px;
        bottom: 1px;
        display: block;
        border-radius: 60px;
        transition: background 0.2s ease-in-out;
        padding-top: 1px;
        font-size: 0.65em;
        letter-spacing: 0.05em;
        background-color: var(--tonic-border, #ccc);
      }

      tonic-toggle .tonic--toggle + label:after {
        content: '';
        width: 16px;
        position: absolute;
        top: 4px;
        left: 4px;
        bottom: 4px;
        background-color: var(--tonic-window, #fff);
        border-radius: 52px;
        transition: background 0.2s ease-in-out, margin 0.2s ease-in-out;
        display: block;
        z-index: 2;
      }

      tonic-toggle .tonic--toggle:disabled {
        cursor: default;
      }

      tonic-toggle .tonic--toggle:disabled + label {
        cursor: default;
        opacity: 0.5;
      }

      tonic-toggle .tonic--toggle:disabled + label:before {
        content: '';
        opacity: 0.5;
      }

      tonic-toggle .tonic--toggle:disabled + label:after {
        background-color: var(--tonic-window, #fff);
      }

      tonic-toggle .tonic--toggle:checked + label {
        background-color: var(--tonic-accent, #f66);
      }

      tonic-toggle .tonic--toggle:checked + label:before {
        background-color: var(--tonic-accent, #f66);
        color: var(--tonic-background);
      }

      tonic-toggle .tonic--toggle:checked + label:after {
        margin-left: 18px;
        background-color: var(--tonic-background, #fff);
      }
    `}change(t){this._setValue(t.target.checked)}renderLabel(){let{id:t,label:e}=this.props;return e?this.html`<label for="tonic--toggle--${t}">${e}</label>`:""}render(){let{id:t,disabled:e,theme:i,tabindex:s}=this.props;s&&this.removeAttribute("tabindex"),i&&this.classList.add(`tonic--theme--${i}`);let o=this.value;return typeof this.state.checked=="undefined"&&(this.state.checked=o),this.html`
      <div class="tonic--toggle--wrapper">
        <div class="tonic--switch">
          <input ... ${{type:"checkbox",class:"tonic--toggle",id:`tonic--toggle--${t}`,disabled:e===!0||e==="true",tabindex:s,checked:o}}/>
          <label for="tonic--toggle--${t}"></label>
        </div>
        ${this.renderLabel()}
      </div>
    `}};a(_t,"TonicToggle");Fe.exports={TonicToggle:_t}});var Xe=v((js,Be)=>{var zt;try{zt=y()}catch(r){throw console.error("Missing dependency. Try `npm install @socketsupply/tonic`."),r}var He=zt.version,Li=He?He.split(".")[0]:"0";if(parseInt(Li,10)<12)throw console.error("Out of date dependency. Try `npm install @socketsupply/tonic@12`."),new Error("Invalid Tonic version. requires at least v12");var{TonicAccordion:Ci,TonicAccordionSection:Ii}=Ot(),{TonicBadge:_i}=Yt(),{TonicButton:zi}=Pt(),{TonicChart:Mi}=Bt(),{TonicCheckbox:Ri}=Ut(),{TonicDialog:Ni}=dt(),{TonicForm:Di}=Jt(),{TonicIcon:Vi}=Qt(),{TonicInput:ji}=ee(),{TonicLoader:Oi}=se(),{TonicPanel:Fi}=ne(),{TonicPopover:Yi}=ce(),{TonicProfileImage:Hi}=de(),{TonicProgressBar:Pi}=ue(),{TonicRange:Ti}=fe(),{TonicRelativeTime:Bi}=ve(),{TonicRouter:Xi}=xe(),{TonicSelect:Ui}=ke(),{TonicSprite:Wi}=Ae(),{TonicSplit:Gi,TonicSplitLeft:Zi,TonicSplitRight:Ji,TonicSplitTop:Ki,TonicSplitBottom:Qi}=Se(),{TonicTabs:ts,TonicTab:es,TonicTabPanel:is}=Ce(),{TonicTextarea:ss}=_e(),{TonicTooltip:os}=Me(),{TonicToasterInline:ns}=De(),{TonicToaster:rs}=Oe(),{TonicToggle:as}=Ye(),Pe=!1;Be.exports=Te;Te.Tonic=zt;function Te(r){Pe||(Pe=!0,r.add(Ci),r.add(Ii),r.add(_i),r.add(zi),r.add(Mi),r.add(Ri),r.add(Ni),r.add(Di),r.add(ji),r.add(Vi),r.add(Oi),r.add(Fi),r.add(Yi),r.add(Hi),r.add(Pi),r.add(Ti),r.add(Bi),r.add(Xi),r.add(Ui),r.add(Wi),r.add(Gi),r.add(Zi),r.add(Ji),r.add(Ki),r.add(Qi),r.add(is),r.add(es),r.add(ts),r.add(ss),r.add(os),r.add(ns),r.add(rs),r.add(as))}a(Te,"components")});var R=y(),cs=Xe();cs(R);var j,P,Q;window.addEventListener("contextmenu",r=>{process.debug||r.preventDefault()});window.addEventListener("dropout",r=>{console.log(r.detail),document.body.removeAttribute("dragging")});window.addEventListener("dropin",r=>{let{x:t,y:e,src:i}=r.detail,s=document.elementFromPoint(t,e);if(!s)throw new Error("No element found at drop point");console.log(s),document.querySelector("#output").value+=i+`
`});window.addEventListener("drag",r=>{j||(j=document.querySelector("#drag-indicator"));let{x:t,y:e,inbound:i}=r.detail;j.style.display=i?"none":"block",j.style.left=`${t+8}px`,j.style.top=`${e+8}px`,j.innerText=r.detail.count,P=document.elementFromPoint(t,e),Q&&Q!==P&&Q.blur(),P&&(Q=P,P.focus()),document.body.hasAttribute("dragging")||document.body.setAttribute("dragging","true")});window.addEventListener("dragend",()=>{setTimeout(()=>{document.body.removeAttribute("dragging"),j.style.display="none"},512)});window.addEventListener("menuItemSelected",r=>{let{title:t,parent:e}=r.detail;if(t)switch(document.querySelector("#menu-selection").value=r.detail.title,t.toLowerCase()==="quit"&&system.exit(0),e.toLowerCase()){case"edit":switch(t.toLowerCase()){case"cut":{document.execCommand("cut");break}case"copy":{document.execCommand("copy");break}case"paste":{document.execCommand("paste");break}case"select all":{document.activeElement&&document.activeElement.select?document.activeElement.select():document.getSelection().extend(document.activeElement);break}case"delete":{if(document.activeElement&&document.activeElement.value&&"selectionStart"in document.activeElement&&"selectionEnd"in document.activeElement){let{selectionStart:i,selectionEnd:s,value:o}=document.activeElement;document.activeElement.value=o.slice(0,i).concat(o.slice(s))}break}}}});window.addEventListener("keyup",async r=>{r.ctrlKey&&r.key==="q"&&system.exit(0)});window.addEventListener("click",async r=>{let t=R.match(r.target,"#externalLink");!t||(r.preventDefault(),await system.openExternal(t.props.url))});window.addEventListener("data",r=>{if(r.detail.env){console.log(r);return}if(r.detail.size!==r.detail.sending.length)throw new Error("Not aligned: detail size not accurate");console.log(`received ${r.detail.size} characters`),tt.setHeader(`${r.detail.counter} messages received`)});var Mt=class extends R{render(){return this.html`
      <h1>${this.props.message}</h1>
    `}};a(Mt,"AppHeader");R.add(Mt);var tt=class extends R{static setHeader(t){document.querySelector("app-header").reRender({message:t})}async click(t){let e=R.match(t.target,"[data-event]");if(!e)return;let{event:i}=e.dataset;i==="open"&&(this.querySelector("#output").value=await system.dialog({allowDirs:!0,allowFiles:!1})),i==="save"&&(this.querySelector("#output").value=await system.dialog({type:"save",defaultPath:"test.txt"})),i==="restart"&&system.send({restart:!0}),i==="showInspector"&&system.showInspector()}async input(t){if(!R.match(t.target,"tonic-input"))return;let i;try{let s={input:t.target.value};i=await system.send(s)}catch(s){console.log(s.message)}this.querySelector("#response").value=i.received.input}async mouseup(t){t.target.id==="drop-demo"&&console.log("drop")}async contextmenu(t){if(!R.match(t.target,".context-menu"))return;t.preventDefault();let i=await system.setContextMenu({Download:"d",Wizard:"w","---":"",Inspect:"i"});i.title==="Inspect"&&system.inspect(),i.title&&(document.querySelector("#menu-selection").value=i.title)}async render(){return this.html`
      <app-header>
      </app-header>
      <!-- <code>{JSON.stringify(window.process)}</code> -->
      <a href="bad-text">BAD ANCHOR</a>
      <a href="https://example.com">FACEBOOK</a>

      <div class="dragdrop-demo">
        <div
          class="draggable"
          data-macsrc="/Users/paolofragomeni/projects/optoolco/opkit/README.md;https://optool.co/images/operator-horizontal.svg;/tmp/test.txt"
          data-src="/tmp/test.txt;/tmp/test2.txt"
        >DRAG HERE</div>

        <div
          class="droppable"
          id="drop-demo"
          tabindex="0"
        >DROP HERE</div>
      </div>

      <div class="grid">
        <tonic-input id="send" label="send">
        </tonic-input>

        <tonic-input id="response" label="recieve" readonly="true">
        </tonic-input>
      </div>

      <tonic-button data-event="open" id="open">Open</tonic-button>
      <tonic-button data-event="save" id="save">Save</tonic-button>
      <tonic-button data-event="restart" id="restart">Restart</tonic-button>

      <tonic-textarea id="output" label="output files/dirs"></tonic-textarea>

      <div class="grid">
        <tonic-input id="menu-selection" readonly="true" label="menu selection">
        </tonic-input>

        <div class="context-menu" draggable="true">
          Context Menu Enabled Area
        </div>
      </div>

      <tonic-button id="externalLink" url="https://example.com">External</tonic-button>
      <!-- tonic-button data-event="showInspector">Inspect</tonic-button -->
      <div id="drag-indicator"></div>
    `}};a(tt,"AppContainer");window.onload=()=>R.add(tt);})();
